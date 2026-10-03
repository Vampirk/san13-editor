#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/CityManager.hpp"
#include <iostream>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [\033[31mFAIL\033[0m] " << msg << " (" << #cond << ")\n"; \
            return false; \
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

    // 3. 주요 도시 개별 로드 및 데이터 검증
    std::cout << "\n[Step 3] 주요 도시(양평, 업, 낙양, 장안, 허창, 건업, 성도) 로드 검증\n";
    CityRecord c0, c_ye, c_luoyang, c_changan, c_xuchang, c_jianye, c_chengdu;
    
    TEST_ASSERT(cityMgr.LoadCity(0, c0), "도시 0 (양평) 로드 성공");
    std::cout << "         -> 양평 (ID 0): 자금=" << c0.gold << ", 군량=" << c0.provisions << ", 민간인구=" << c0.civilian_population << ", 병력=" << c0.troops << ", 총인구=" << c0.total_population << "\n";
    TEST_ASSERT(c0.gold == 2200 && c0.provisions == 4240 && c0.civilian_population == 95000 && c0.troops == 2120, "양평 실시간 자원 데이터(자금 2,200, 군량 4,240, 민간인구 95,000, 병력 2,120) 일치");
    TEST_ASSERT(c0.total_population == 97120 && c0.GetTotalPopulation() == 97120, "양평 총 인구 공식(95,000 + 2,120 = 97,120) 일치");
    TEST_ASSERT(c0.settlement_count == 1 && c0.settlement_ids[0] == 1, "양평 관할 부락(1개, 부락 ID 1) 일치");

    TEST_ASSERT(cityMgr.LoadCity(9, c_ye), "도시 9 (업) 로드 성공");
    std::cout << "         -> 업 (ID 9): 자금=" << c_ye.gold << ", 군량=" << c_ye.provisions << ", 민간인구=" << c_ye.civilian_population << ", 병력=" << c_ye.troops << ", 총인구=" << c_ye.total_population << "\n";
    TEST_ASSERT(c_ye.gold == 2800 && c_ye.provisions == 24000 && c_ye.civilian_population == 108000 && c_ye.troops == 12000, "업 실시간 자원 데이터(자금 2,800, 군량 24,000, 민간인구 108,000, 병력 12,000) 일치");
    TEST_ASSERT(c_ye.total_population == 120000 && c_ye.GetTotalPopulation() == 120000, "업 총 인구 공식(108,000 + 12,000 = 120,000) 일치");

    TEST_ASSERT(cityMgr.LoadCity(23, c_luoyang), "도시 23 (낙양) 로드 성공");
    std::cout << "         -> 낙양 (ID 23): 자금=" << c_luoyang.gold << ", 군량=" << c_luoyang.provisions << ", 민간인구=" << c_luoyang.civilian_population << ", 병력=" << c_luoyang.troops << ", 총인구=" << c_luoyang.total_population << "\n";
    TEST_ASSERT(c_luoyang.gold == 4000 && c_luoyang.provisions == 44400 && c_luoyang.civilian_population == 200000 && c_luoyang.troops == 22200, "낙양 실시간 자원 데이터(자금 4,000, 군량 44,400, 민간인구 200,000, 병력 22,200) 일치");
    TEST_ASSERT(c_luoyang.total_population == 222200 && c_luoyang.GetTotalPopulation() == 222200, "낙양 총 인구 공식(200,000 + 22,200 = 222,200) 일치");
    TEST_ASSERT(c_luoyang.settlement_count == 4, "낙양 관할 부락(4개) 일치");

    TEST_ASSERT(cityMgr.LoadCity(20, c_changan), "도시 20 (장안) 로드 성공");
    TEST_ASSERT(cityMgr.LoadCity(25, c_xuchang), "도시 25 (허창) 로드 성공");
    TEST_ASSERT(cityMgr.LoadCity(39, c_jianye), "도시 39 (건업) 로드 성공");
    TEST_ASSERT(cityMgr.LoadCity(46, c_chengdu), "도시 46 (성도) 로드 성공");

    // 3-2. 신규 발굴 내정/숙련도/인물 통계 필드 검증 (양평 0번, 낙양 23번)
    std::cout << "\n[Step 3-2] 신규 발굴 내정/숙련도/인물 통계 필드 검증\n";
    TEST_ASSERT(c0.public_order == 100, "양평 민심 100 실측 일치");
    TEST_ASSERT(c0.commerce == 1000, "양평 상업 기본치 1,000 실측 일치");
    TEST_ASSERT(c0.agriculture == 1050, "양평 농업 기본치 1,050 실측 일치");
    TEST_ASSERT(c0.culture == 1000, "양평 문화 기본치 1,000 실측 일치");
    TEST_ASSERT(c0.spear_proficiency == 200, "양평 창병 숙련도 200 실측 일치");
    TEST_ASSERT(c0.horse_proficiency == 300, "양평 기병 숙련도 300 실측 일치");
    TEST_ASSERT(c0.bow_proficiency == 200, "양평 궁병 숙련도 200 실측 일치");

    TEST_ASSERT(c_luoyang.public_order == 350, "낙양 민심 350 실측 일치");
    TEST_ASSERT(c_luoyang.commerce == 1800, "낙양 상업 기본치 1,800 실측 일치");
    TEST_ASSERT(c_luoyang.agriculture == 2000, "낙양 농업 기본치 2,000 실측 일치");
    TEST_ASSERT(c_luoyang.culture == 2400, "낙양 문화 기본치 2,400 실측 일치");
    TEST_ASSERT(c_luoyang.spear_proficiency == 700, "낙양 창병 숙련도 700 실측 일치");
    TEST_ASSERT(c_luoyang.horse_proficiency == 700, "낙양 기병 숙련도 700 실측 일치");
    TEST_ASSERT(c_luoyang.bow_proficiency == 700, "낙양 궁병 숙련도 700 실측 일치");
    TEST_ASSERT(c_luoyang.officer_count == 12, "낙양 소속 무장 수 12명 실측 일치");
    TEST_ASSERT(c_luoyang.prisoner_count == 0, "낙양 수감 포로 수 0명 실측 일치");
    TEST_ASSERT(c_luoyang.free_officer_count == 1, "낙양 재야 무장 수 1명 실측 일치");

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
