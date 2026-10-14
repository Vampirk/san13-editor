#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/CityManager.hpp"
#include <iostream>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [\033[31mFAIL\033[0m] " << msg << " (" << #cond << ")\n"; \
            return 1; /* main()에서 false는 exit 0(성공)이 되므로 1 반환 (TECH-05) */ \
        } else { \
            std::cout << "  [\033[32mPASS\033[0m] " << msg << "\n"; \
        } \
    } while (0)

using namespace San13::Core;
using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 4.4] City Manager Domain Unit Test Suite\n";
    std::cout << "============================================================\n\n";

    // 1. 드라이버 및 프로세스 부착
    std::cout << "[Step 1] 드라이버 부착 및 RootBase 산출\n";
    LinuxMemoryDriver driver;
    bool attached = driver.Attach("San13PK_kr.exe") || driver.Attach("san13pk");
    if (!attached) {
        std::cerr << "[-] Error: 게임 프로세스를 찾을 수 없습니다.\n";
        return 1;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    bool verOk = GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);
    TEST_ASSERT(verOk, "RootBase 산출 성공");

    // 2. CityManager 초기화
    std::cout << "\n[Step 2] CityManager 초기화\n";
    CityManager cityMgr;
    bool initOk = cityMgr.Initialize(&driver, rootBase);
    TEST_ASSERT(initOk, "CityManager 초기화 성공");

    // 3. 주요 도시 개별 로드 및 데이터 정합성 검증 (TECH-05)
    // 특정 세이브의 실측 스냅샷 값(양평 자금 2,200 등)은 세이브마다 달라 라이브 단언으로 쓸 수 없으므로
    // 세이브와 무관한 구조적 불변식만 검증합니다. 실측 증거는 docs/quality/VERIFICATION.md §3.5에 보존됩니다.
    std::cout << "\n[Step 3] 주요 도시(양평, 업, 낙양, 장안, 허창, 건업, 성도) 로드 및 정합성 검증\n";
    CityRecord c0, c_ye, c_luoyang, c_changan, c_xuchang, c_jianye, c_chengdu;

    // 세이브와 무관하게 성립해야 하는 도시 레코드 불변식
    auto cityInvariantsOk = [](const CityRecord& c) -> bool {
        constexpr uint32_t RESOURCE_SANITY_MAX = 10000000;  // LimitBreak 상한(9,999,999) 이내
        bool ok = c.gold < RESOURCE_SANITY_MAX && c.provisions < RESOURCE_SANITY_MAX &&
                  c.civilian_population < RESOURCE_SANITY_MAX && c.troops < RESOURCE_SANITY_MAX;
        ok = ok && c.total_population == c.civilian_population + c.troops &&
             c.GetTotalPopulation() == c.total_population;
        ok = ok && c.public_order <= 5000 && c.commerce <= 10000 && c.agriculture <= 10000 && c.culture <= 10000;
        ok = ok && c.spear_proficiency <= 5000 && c.horse_proficiency <= 5000 && c.bow_proficiency <= 5000;
        ok = ok && c.officer_count <= 200 && c.prisoner_count <= 200 && c.free_officer_count <= 200;
        return ok;
    };

    // 도시 ID는 EDGE-42 공식 순서 기준 (7=업, 20=허창, 23=낙양, 25=장안, 34=성도, 52=건업)
    const struct { uint8_t id; const char* name; CityRecord* rec; } majorCities[] = {
        { 0, "양평", &c0 }, { 7, "업", &c_ye }, { 23, "낙양", &c_luoyang }, { 25, "장안", &c_changan },
        { 20, "허창", &c_xuchang }, { 52, "건업", &c_jianye }, { 34, "성도", &c_chengdu },
    };
    for (const auto& mc : majorCities) {
        TEST_ASSERT(cityMgr.LoadCity(mc.id, *mc.rec), std::string("도시 ") + std::to_string(mc.id) + " (" + mc.name + ") 로드 성공");
        const CityRecord& c = *mc.rec;
        TEST_ASSERT(c.name.rfind(mc.name, 0) == 0, std::string("도시 ") + std::to_string(mc.id) + " 명칭 '" + mc.name + "' 일치 (실제: " + c.name + ")");
        std::cout << "         -> " << mc.name << " (ID " << (int)mc.id << "): 자금=" << c.gold << ", 군량=" << c.provisions
                  << ", 민간인구=" << c.civilian_population << ", 병력=" << c.troops << ", 총인구=" << c.total_population
                  << ", 민심=" << c.public_order << ", 상업=" << c.commerce << ", 농업=" << c.agriculture << ", 문화=" << c.culture
                  << ", 숙련도=" << c.spear_proficiency << "/" << c.horse_proficiency << "/" << c.bow_proficiency
                  << ", 인물(현역/포로/재야)=" << c.officer_count << "/" << c.prisoner_count << "/" << c.free_officer_count << "\n";
        TEST_ASSERT(cityInvariantsOk(c), std::string(mc.name) + " 자원·인구 공식·내정·숙련도·인물 수 범위 정합성 확인");
    }

    // 지리 정보는 시나리오와 무관한 고정값
    TEST_ASSERT(c0.settlement_count == 1 && c0.connected_city_ids[0] == 1, "양평 관할 부락 수(1개) 및 인접 도시(북평 ID 1) 일치");
    TEST_ASSERT(c_luoyang.settlement_count == 4, "낙양 관할 부락(4개) 일치");

    // 4. 도시 자원 쓰기 및 Bounds Clamping 검증 (낙양 ID 23 기준)
    std::cout << "\n[Step 4] 도시 자원 실시간 수정 및 Bounds Clamping 검증 (낙양 ID 23)\n";
    uint32_t origGold = c_luoyang.gold;
    uint32_t origProv = c_luoyang.provisions;

    // 4-1. SafeVanilla 모드: 100만 초과(2,000,000) 입력 시 999,999로 절삭
    cityMgr.SetCityGold(23, 2000000, ClampingPolicy::SafeVanilla);
    cityMgr.LoadCity(23, c_luoyang);
    TEST_ASSERT(c_luoyang.gold == 999999, "Safe 모드: 200만 입력 시 999,999로 클램핑 확인");

    // 4-2. LimitBreak 모드: 1,500,000 입력 허용
    cityMgr.SetCityProvisions(23, 1500000, ClampingPolicy::LimitBreak);
    cityMgr.LoadCity(23, c_luoyang);
    TEST_ASSERT(c_luoyang.provisions == 1500000, "LimitBreak 모드: 150만 군량 입력 허용 확인");

    // 4-3. 음수 입력 시: 0으로 안전 보정
    cityMgr.SetCityGold(23, -500, ClampingPolicy::SafeVanilla);
    cityMgr.LoadCity(23, c_luoyang);
    TEST_ASSERT(c_luoyang.gold == 0, "음수 자금 입력 시 0으로 안전 보정 확인");

    // 4-4. 원클릭 자원 최대화 치트 (MaximizeCityResources)
    cityMgr.MaximizeCityResources(23);
    cityMgr.LoadCity(23, c_luoyang);
    TEST_ASSERT(c_luoyang.gold == 999999 && c_luoyang.provisions == 999999,
                "MaximizeCityResources 치트 실행 시 자금/군량 999,999 완충 확인");

    // 4-5. 테스트 후 원래 값으로 복구
    cityMgr.SetCityGold(23, origGold);
    cityMgr.SetCityProvisions(23, origProv);
    cityMgr.LoadCity(23, c_luoyang);
    TEST_ASSERT(c_luoyang.gold == origGold && c_luoyang.provisions == origProv, "낙양 자원 원상 복구 완료");

    // 5. 60개 도시 전원 일괄 파싱 및 메모리 무결성 검증
    std::cout << "\n[Step 5] 60개 도시 전원 일괄 파싱 및 메모리 무결성 검증\n";
    std::vector<CityRecord> allCities;
    bool allOk = cityMgr.LoadAllCities(allCities);
    TEST_ASSERT(allOk, "60개 도시 일괄 로드 성공");
    TEST_ASSERT(allCities.size() == 60, "정확히 60개 도시 레코드 수집 확인");

    bool continuous = true;
    for (size_t i = 0; i < allCities.size(); i++) {
        if (allCities[i].id != i) {
            continuous = false;
            break;
        }
    }
    TEST_ASSERT(continuous, "0부터 59까지 도시 ID 완전 연속성 확인");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 모든 도시 도메인 로직 및 60개 도시 로드 검증 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 4.4 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";

    return 0;
}
