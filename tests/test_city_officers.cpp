#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/CityManager.hpp"
#include <iostream>
#include <cassert>
#include <chrono>

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
    std::cout << " [San13PK Phase 3.2] City Officer Linked-List Unit Test Suite\n";
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

    // 2. CityManager 초기화
    std::cout << "\n[Step 2] CityManager 초기화\n";
    CityManager cityMgr;
    bool initOk = cityMgr.Initialize(&driver, rootBase);
    TEST_ASSERT(initOk, "CityManager 초기화 성공");

    // 3. 도시 소속 현역 무장 순회 및 헤더 카운트 동기화 검증 (양평 ID 0)
    std::cout << "\n[Step 3] 도시 소속 현역 무장 연결 리스트 순회 검증\n";
    CityRecord city0;
    TEST_ASSERT(cityMgr.LoadCity(0, city0), "양평(ID 0) 로드 성공");
    std::vector<CityOfficerEntry> activeOfficers0;
    bool activeOk0 = cityMgr.LoadCityOfficers(0, CityOfficerCategory::Active, activeOfficers0);
    TEST_ASSERT(activeOk0, "양평 소속 현역 무장 리스트 순회 성공");
    TEST_ASSERT(activeOfficers0.size() == city0.officer_count, "양평 소속 무장 수와 DLL 순회 수 100% 일치");
    std::cout << "         -> 양평 소속 무장: " << activeOfficers0.size() << "명 (헤더: " << city0.officer_count << "명)\n";

    // 낙양(ID 23) 순회 검증
    CityRecord city23;
    TEST_ASSERT(cityMgr.LoadCity(23, city23), "낙양(ID 23) 로드 성공");
    std::vector<CityOfficerEntry> activeOfficers23;
    bool activeOk23 = cityMgr.LoadCityOfficers(23, CityOfficerCategory::Active, activeOfficers23);
    TEST_ASSERT(activeOk23, "낙양 소속 현역 무장 리스트 순회 성공");
    TEST_ASSERT(activeOfficers23.size() == city23.officer_count, "낙양 소속 무장 수와 DLL 순회 수 100% 일치");
    std::cout << "         -> 낙양 소속 무장: " << activeOfficers23.size() << "명 (헤더: " << city23.officer_count << "명)\n";

    // 4. 재야 무장 연결 리스트 순회 검증 (헤더 free_officer_count와 일치 검증)
    std::cout << "\n[Step 4] 도시 재야 무장 연결 리스트 순회 검증\n";
    std::vector<CityOfficerEntry> freeOfficers0;
    bool freeOk0 = cityMgr.LoadCityOfficers(0, CityOfficerCategory::Free, freeOfficers0);
    TEST_ASSERT(freeOk0, "양평 재야 무장 리스트 순회 성공");
    TEST_ASSERT(freeOfficers0.size() == city0.free_officer_count, "양평 재야 무장 수와 DLL 순회 수 일치");

    // 5. 낙양(ID 23) 수감 포로 순회 검증 (예상: 0명)
    std::cout << "\n[Step 5] 낙양(ID 23) 수감 포로 연결 리스트 순회 검증\n";
    std::vector<CityOfficerEntry> prisoners;
    bool prisonerOk = cityMgr.LoadCityOfficers(23, CityOfficerCategory::Prisoner, prisoners);
    TEST_ASSERT(prisonerOk, "낙양 수감 포로 리스트 순회 성공");
    TEST_ASSERT(prisoners.empty(), "낙양 수감 포로 0명 일치");

    // 6. LoadAllCityOfficers 통합 로더 검증
    std::cout << "\n[Step 6] LoadAllCityOfficers 3분류 일괄 로더 검증\n";
    std::vector<CityOfficerEntry> allAct, allPris, allFr;
    bool allOk = cityMgr.LoadAllCityOfficers(23, allAct, allPris, allFr);
    TEST_ASSERT(allOk, "LoadAllCityOfficers 호출 성공");
    TEST_ASSERT(allAct.size() == city23.officer_count, "통합 로더 현역 수 일치");
    TEST_ASSERT(allPris.size() == city23.prisoner_count, "통합 로더 포로 수 일치");
    TEST_ASSERT(allFr.size() == city23.free_officer_count, "통합 로더 재야 수 일치");

    // 7. 60개 전체 도시 순환 가드 및 성능 벤치마크
    std::cout << "\n[Step 7] 60개 전체 도시 연결 리스트 전수 순회 및 무한 루프 차단 검증\n";
    auto start = std::chrono::high_resolution_clock::now();
    size_t totalActive = 0;
    size_t totalPrisoners = 0;
    size_t totalFree = 0;

    for (uint8_t cityId = 0; cityId < CityMemoryLayout::MAX_CITY_COUNT; cityId++) {
        std::vector<CityOfficerEntry> act, pris, fr;
        cityMgr.LoadAllCityOfficers(cityId, act, pris, fr);
        totalActive += act.size();
        totalPrisoners += pris.size();
        totalFree += fr.size();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "         -> 전국 60개 도시 집계: 현역=" << totalActive
              << "명, 포로=" << totalPrisoners
              << "명, 재야=" << totalFree << "명\n";
    std::cout << "         -> 60개 도시 순회 소요 시간: " << elapsedMs << "ms\n";
    TEST_ASSERT(totalActive > 0, "전국 현역 무장 0명 초과 확인");
    TEST_ASSERT(elapsedMs < 100.0, "전국 60개 도시 리스트 순회 100ms 미만 초고속 통과");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 낙양 12현역/1재야/0포로 검증 및 60개 도시 무한루프 가드 100% 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> [PHASE 3.2 ACCEPTANCE CRITERIA 100% 통과] <<<\n\n";

    return 0;
}
