#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/CityManager.hpp"
#include "game/SettlementManager.hpp"
#include <iostream>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [\033[31mFAIL\033[0m] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } else { \
            std::cout << "  [\033[32mPASS\033[0m] " << msg << "\n"; \
        } \
    } while (0)

using namespace San13::Core;
using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 3.1] Settlement Manager Domain Unit Test Suite\n";
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
    std::cout << "         -> RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    // 2. SettlementManager & CityManager 초기화
    std::cout << "\n[Step 2] SettlementManager 초기화\n";
    SettlementManager setMgr;
    bool initOk = setMgr.Initialize(&driver, rootBase);
    TEST_ASSERT(initOk, "SettlementManager 초기화 성공");

    CityManager cityMgr;
    TEST_ASSERT(cityMgr.Initialize(&driver, rootBase), "CityManager 초기화 성공");

    // 3. 단일 부락 로드 검증
    std::cout << "\n[Step 3] 단일 부락(0번, 1번, 6번, 117번) 개별 로드 및 공식 인게임 지명/효과 검증\n";
    SettlementRecord s0, s1, s6, s117;
    TEST_ASSERT(setMgr.LoadSettlement(0, s0), "부락 0 로드 성공");
    TEST_ASSERT(s0.id == 0, "부락 0 ID 일치");
    TEST_ASSERT(s0.name == "범", "부락 0 공식 지명 '범' 일치");
    TEST_ASSERT(!s0.effect_name.empty() && s0.effect_name != "-", "부락 0 효과 로드 일치");
    std::cout << "         -> 부락 0: ID=" << s0.id << ", 지명=" << s0.name << ", 효과=" << s0.effect_name << ", 세력=" << s0.force_id << ", 장악도=" << s0.dev_progress << "%\n";

    TEST_ASSERT(setMgr.LoadSettlement(1, s1), "부락 1 로드 성공");
    TEST_ASSERT(s1.id == 1, "부락 1 ID 일치");
    TEST_ASSERT(s1.name == "동평", "부락 1 공식 지명 '동평' 일치");
    TEST_ASSERT(!s1.effect_name.empty() && s1.effect_name != "-", "부락 1 효과 로드 일치");
    std::cout << "         -> 부락 1: ID=" << s1.id << ", 지명=" << s1.name << ", 효과=" << s1.effect_name << ", 세력=" << s1.force_id << ", 장악도=" << s1.dev_progress << "%\n";

    TEST_ASSERT(setMgr.LoadSettlement(6, s6), "부락 6 로드 성공");
    TEST_ASSERT(s6.id == 6, "부락 6 ID 일치");
    TEST_ASSERT(s6.name == "밀", "부락 6 공식 지명 '밀' 일치");
    TEST_ASSERT(!s6.effect_name.empty() && s6.effect_name != "-", "부락 6 효과 로드 확인");
    std::cout << "         -> 부락 6: ID=" << s6.id << ", 지명=" << s6.name << ", 효과=" << s6.effect_name << "\n";

    TEST_ASSERT(setMgr.LoadSettlement(117, s117, 0, "양평"), "부락 117 (양평 관할) 로드 성공");
    TEST_ASSERT(s117.id == 117, "부락 117 ID 일치");
    TEST_ASSERT(s117.city_id == 0, "부락 117 양평(0) 관할 확인");
    TEST_ASSERT(s117.name == "임유", "부락 117 공식 지명 '임유' 일치");
    TEST_ASSERT(s117.effect_name.find("호표기") != std::string::npos, "부락 117 특수병과 '호표기' 편제 확인");
    std::cout << "         -> 부락 117: ID=" << s117.id << ", 지명=" << s117.name << ", 효과=" << s117.effect_name << ", 세력=" << s117.force_id << ", 장악도=" << s117.dev_progress << "%\n";

    // 4. 도시와 부락 연동(LoadCitySettlements) 검증
    std::cout << "\n[Step 4] 도시별 관할 부락 역방향 매핑(양평 0번, 낙양 23번) 검증\n";
    CityRecord yangpyeong, luoyang;
    TEST_ASSERT(cityMgr.LoadCity(0, yangpyeong), "도시 0 (양평) 로드");
    TEST_ASSERT(cityMgr.LoadCity(23, luoyang), "도시 23 (낙양) 로드");

    std::vector<SettlementRecord> ypSettlements;
    TEST_ASSERT(setMgr.LoadCitySettlements(0, yangpyeong.name, ypSettlements),
                "양평 관할 부락 로드 성공");
    TEST_ASSERT(ypSettlements.size() == 1, "양평 관할 부락 정확히 1개 로드 확인");
    TEST_ASSERT(ypSettlements[0].id == 117, "양평 관할 부락 ID 117 일치");
    TEST_ASSERT(ypSettlements[0].name == "임유", "양평 관할 부락 지명 '임유' 일치");
    TEST_ASSERT(ypSettlements[0].effect_name.find("호표기") != std::string::npos, "양평 관할 부락 '호표기' 특수병과 확인");
    TEST_ASSERT(ypSettlements[0].city_id == 0, "부락 레코드 내 city_id=0 바인딩 확인");
    std::cout << "         -> 양평 관할 부락 바인딩 확인: " << ypSettlements[0].name << " (" << ypSettlements[0].effect_name << ", ID " << ypSettlements[0].id << ")\n";

    std::vector<SettlementRecord> lySettlements;
    TEST_ASSERT(setMgr.LoadCitySettlements(23, luoyang.name, lySettlements),
                "낙양 관할 부락 로드 성공");
    TEST_ASSERT(lySettlements.size() == 4, "낙양 관할 부락 정확히 4개 로드 확인");
    TEST_ASSERT(lySettlements[0].id == 6 && lySettlements[0].name == "밀", "낙양 1번 관할 부락: 밀 (ID 6)");
    TEST_ASSERT(lySettlements[1].id == 9 && lySettlements[1].name == "온", "낙양 2번 관할 부락: 온 (ID 9)");
    TEST_ASSERT(lySettlements[2].id == 35 && lySettlements[2].name == "하남", "낙양 3번 관할 부락: 하남 (ID 35)");
    TEST_ASSERT(lySettlements[3].id == 144 && lySettlements[3].name == "신안", "낙양 4번 관할 부락: 신안 (ID 144)");
    for (const auto& s : lySettlements) {
        TEST_ASSERT(s.city_id == 23, "낙양 관할 부락 city_id=23 일치");
        TEST_ASSERT(s.dev_progress <= 100, "부락 장악도 0~100% 범위 확인");
        std::cout << "         -> 낙양 부락: " << s.name << " (ID " << s.id << ", 장악도=" << s.dev_progress << "%)\n";
    }

    // 5. 300개 부락 전수 로드 및 메모리 무결성 검증
    std::cout << "\n[Step 5] 300개 전체 부락 일괄 로드 및 ID 연속성 검증\n";
    std::vector<SettlementRecord> allSettlements;
    bool allOk = setMgr.LoadAllSettlements(allSettlements);
    TEST_ASSERT(allOk, "300개 전체 부락 일괄 로드 성공");
    TEST_ASSERT(allSettlements.size() == 300, "정확히 300개 부락 수집 확인");

    bool continuous = true;
    for (size_t i = 0; i < allSettlements.size(); i++) {
        if (allSettlements[i].id != i) {
            continuous = false;
            break;
        }
    }
    TEST_ASSERT(continuous, "0부터 299까지 부락 ID 완전 연속성 확인");

    // 6. Phase 4 보완: 부락 성채(건물) 건축, 요충 최대 Lv 및 primary_city_id 동기화 검증
    std::cout << "\n[Step 6] Phase 4: 성채(건물) 건축, 최대 Lv 승급 및 지배 도시 바인딩 검증\n";
    // 6-1. 단일 부락(117번 임유)을 최고 등급 방채(防砦, Lv5)로 건축 (양평 0번 관할 바인딩)
    bool fOk = setMgr.FortifySettlement(117, FortressType::Stronghold, 2500, 0);
    TEST_ASSERT(fOk, "부락 117 [방채(Lv5)] 건축 및 양평 바인딩 성공");

    SettlementRecord f117;
    TEST_ASSERT(setMgr.LoadSettlement(117, f117, 0, "양평"), "건축 후 부락 117 로드 성공");
    TEST_ASSERT(f117.is_fortified == true, "부락 117 건물 건축 플래그 활성 확인");
    TEST_ASSERT(f117.fortress_type == FortressType::Stronghold, "부락 117 성채 유형 '방채' 확인");
    TEST_ASSERT(f117.type_tag == "[방채]", "부락 117 뱃지 태그 '[방채]' 일치");
    TEST_ASSERT(f117.hp == 2500 && f117.max_hp == 2500, "부락 117 내구도 2,500/2,500 확인");
    TEST_ASSERT(f117.max_fortress_level >= 5, "부락 117 최대 요충 레벨 Lv5 확인");
    TEST_ASSERT(f117.dev_progress == 100, "건물 건축으로 인한 장악도 100% 자동 달성 확인");

    // [핵심 검증] primary_city_id 실측 확인
    San13SettlementRaw raw117;
    driver.Read<San13SettlementRaw>(setMgr.GetSettlementAddress(117), raw117);
    TEST_ASSERT(raw117.primary_city_id == 0, "인게임 메모리 primary_city_id가 양평(0)으로 즉시 바인딩됨 확인");

    // [CChiiki 실측 검증] PK 엔진 진짜 성채 구조체(0xD31A10) 쓰기 확인
    uintptr_t chiiki117Addr = setMgr.GetChiikiAddress(117);
    uint8_t c117Buf[20];
    driver.ReadRaw(chiiki117Addr, c117Buf, 20);
    int8_t c117Type = static_cast<int8_t>(c117Buf[7]);
    uint16_t c117Hp = *reinterpret_cast<uint16_t*>(&c117Buf[8]);
    uint8_t c117Lv = c117Buf[0x10];
    TEST_ASSERT(c117Type == 5, "CChiiki jousai_type == 5 (방채) 인게임 엔진 쓰기 확인");
    TEST_ASSERT(c117Hp == 2500, "CChiiki hp == 2500 인게임 내구도 쓰기 확인");
    TEST_ASSERT(c117Lv >= 5, "CChiiki level >= 5 인게임 요충 레벨 승급 확인");
    std::cout << "         -> 건축 확인: " << f117.name << " " << f117.type_tag << " (내구: " << f117.hp << "/" << f117.max_hp << ", 지배도시=" << (int)raw117.primary_city_id << ", CChiiki Type=" << (int)c117Type << ")\n";

    // 6-2. 성채 유형 견진(堅陣, Lv1)으로 전환
    bool cOk = setMgr.FortifySettlement(117, FortressType::Camp, 1000, 0);
    TEST_ASSERT(cOk, "부락 117 [견진]으로 전환 성공");
    SettlementRecord c117;
    setMgr.LoadSettlement(117, c117, 0, "양평");
    TEST_ASSERT(c117.fortress_type == FortressType::Camp, "부락 117 유형 '견진' 전환 확인");
    TEST_ASSERT(c117.type_tag == "[견진]", "부락 117 뱃지 태그 '[견진]' 일치");
    TEST_ASSERT(c117.hp == 1000, "부락 117 내구도 1,000 확인");

    // 6-3. 성채 해체 및 일반 부락 원복
    bool dOk = setMgr.DemolishFortress(117);
    TEST_ASSERT(dOk, "부락 117 성채 해체(원복) 성공");

    driver.ReadRaw(chiiki117Addr, c117Buf, 20);
    c117Type = static_cast<int8_t>(c117Buf[7]);
    c117Hp = *reinterpret_cast<uint16_t*>(&c117Buf[8]);
    TEST_ASSERT(c117Type == -1, "CChiiki jousai_type == -1 (해체) 인게임 엔진 원복 확인");
    TEST_ASSERT(c117Hp == 0xFFFF, "CChiiki hp == 0xFFFF (미건축 내구도) 원복 확인");

    SettlementRecord d117;
    setMgr.LoadSettlement(117, d117, 0, "양평");
    TEST_ASSERT(d117.is_fortified == false, "부락 117 건물 건축 플래그 비활성 확인");
    TEST_ASSERT(d117.fortress_type == FortressType::None, "부락 117 성채 유형 'None' 원복 확인");
    TEST_ASSERT(d117.type_tag == "[부락]", "부락 117 뱃지 태그 '[부락]' 원복 확인");
    std::cout << "         -> 원복 확인: " << d117.name << " " << d117.type_tag << " (일반 부락 복귀)\n";

    // 6-4. 도시 관할 전체 일괄 방채(Lv5) 요새화 치트 검증 (낙양 23번)
    std::cout << "\n[Step 6-4] 도시 관할 전체 부락 일괄 방채(Lv5) 요새화 치트 검증 (낙양 23번)\n";
    bool allFortOk = setMgr.FortifyAllCitySettlements(23, FortressType::Stronghold);
    TEST_ASSERT(allFortOk, "낙양 관할 부락 전체 방채 일괄 요새화 성공");
    std::vector<SettlementRecord> lyForts;
    setMgr.LoadCitySettlements(23, "낙양", lyForts);
    for (const auto& s : lyForts) {
        TEST_ASSERT(s.is_fortified == true, "낙양 관할 부락 건물 건축 확인");
        TEST_ASSERT(s.fortress_type == FortressType::Stronghold, "낙양 관할 부락 방채(Lv5) 유형 확인");
        TEST_ASSERT(s.hp == 2500, "낙양 관할 부락 내구도 2,500 확인");
        TEST_ASSERT(s.max_fortress_level == 5, "낙양 관할 부락 최대 레벨 Lv5 승급 확인");

        San13SettlementRaw lyRaw;
        driver.Read<San13SettlementRaw>(setMgr.GetSettlementAddress(s.id), lyRaw);
        TEST_ASSERT(lyRaw.primary_city_id == 23, "낙양 부락 primary_city_id=23 낙양 지배 깃발 바인딩 확인");
        std::cout << "         -> 낙양 관할: " << s.name << " " << s.type_tag << " (내구: " << s.hp << "/" << s.max_hp << ", primary=" << (int)lyRaw.primary_city_id << ")\n";
    }

    // 6-5. 게임 데이터 보존을 위해 낙양 부락 원래 상태로 해체 원복
    for (const auto& s : lyForts) {
        setMgr.DemolishFortress(s.id);
    }
    std::cout << "         -> 낙양 관할 부락 원래 일반 부락으로 원복 완료\n";

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 성채 건축, 최대 Lv 승급, primary_city_id 바인딩 100% 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 4 USER FEEDBACK RESOLUTION 100% 통과]\033[0m <<<\n\n";

    return 0;
}
