#include "ui/UIManager.hpp"
#include "ui/CityTableView.hpp"
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/CityManager.hpp"
#include "game/SettlementManager.hpp"
#include "imgui.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
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
using namespace San13::UI;

int main(int argc, char** argv) {
    bool smokeTest = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--smoke-test" || arg == "--auto-close") {
            smokeTest = true;
        }
    }

    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 5.4] City Resource Editor & Table View Test\n";
    std::cout << "============================================================\n\n";

    // 1. 메모리 드라이버 및 CityManager 초기화
    std::cout << "[Step 1] 게임 프로세스 연결 및 CityManager 초기화\n";
    LinuxMemoryDriver driver;
    bool attached = driver.Attach("San13PK_kr.exe") || driver.Attach("san13pk");
    if (!attached) {
        std::cerr << "[-] Error: 삼국지 13 PK 프로세스를 찾을 수 없습니다.\n";
        return 1;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    bool verOk = GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);
    TEST_ASSERT(verOk, "버전 감지 및 RootBase 산출 성공");
    std::cout << "         -> RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    CityManager cityMgr;
    bool mgrOk = cityMgr.Initialize(&driver, rootBase);
    TEST_ASSERT(mgrOk, "CityManager 초기화 성공");

    SettlementManager setMgr;
    TEST_ASSERT(setMgr.Initialize(&driver, rootBase), "SettlementManager 초기화 성공");

    // 2. CityTableView 초기화 및 60개 도시 일괄 로드
    std::cout << "\n[Step 2] CityTableView 초기화 및 60개 도시 데이터 바인딩\n";
    CityTableView tableView;
    tableView.Initialize(&cityMgr, &setMgr);

    size_t totalCities = tableView.GetTotalCount();
    std::cout << "         -> 총 로드된 도시 수: " << totalCities << "개\n";
    TEST_ASSERT(totalCities == 60, "정확히 60개 전체 도시 레코드 로드 확인");
    TEST_ASSERT(tableView.GetFilteredCount() == 60, "초기 전체 필터 카운트 일치");

    const auto* firstCity = tableView.GetCityAt(0);
    TEST_ASSERT(firstCity != nullptr && firstCity->id == 0 && firstCity->name.find("양평") != std::string::npos, 
                "0번 도시 양평(ID 0) 로드 일치 확인");

    // 3. 다중 컬럼 정렬 기능 검증
    std::cout << "\n[Step 3] 실시간 다중 컬럼 정렬 기능 검증\n";

    // 3-1. 자금(Column 2) 내림차순 정렬
    tableView.SortByColumn(2, false);
    const auto* topGold = tableView.GetCityAt(0);
    TEST_ASSERT(topGold != nullptr && topGold->gold >= 2000, 
                "자금 내림차순 정렬: 1위 도시 자금 검증 (1위: " + (topGold ? topGold->name : "") + 
                ", 자금: " + std::to_string(topGold ? topGold->gold : 0) + ")");

    // 3-2. 군량(Column 3) 내림차순 정렬
    tableView.SortByColumn(3, false);
    const auto* topProv = tableView.GetCityAt(0);
    TEST_ASSERT(topProv != nullptr && topProv->provisions >= 20000, 
                "군량 내림차순 정렬: 1위 도시 군량 검증 (1위: " + (topProv ? topProv->name : "") + 
                ", 군량: " + std::to_string(topProv ? topProv->provisions : 0) + ")");

    // 3-3. 병력(Column 4) 내림차순 정렬
    tableView.SortByColumn(4, false);
    const auto* topTroops = tableView.GetCityAt(0);
    TEST_ASSERT(topTroops != nullptr && topTroops->troops >= 10000, 
                "병력 내림차순 정렬: 1위 도시 병력 검증 (1위: " + (topTroops ? topTroops->name : "") + ")");

    // 3-4. ID(Column 0) 오름차순 정렬 복귀
    tableView.SortByColumn(0, true);
    const auto* topId = tableView.GetCityAt(0);
    TEST_ASSERT(topId != nullptr && topId->id == 0, "ID 오름차순 정렬 복귀 확인 (0번 양평)");

    // 4. 실시간 한글 및 ID 검색 필터 검증
    std::cout << "\n[Step 4] 실시간 한글 및 ID 검색 필터 검증\n";

    // 4-1. "낙양" 검색
    tableView.SetSearchQuery("낙양");
    TEST_ASSERT(tableView.GetFilteredCount() == 1, "한글 '낙양' 검색 결과 정확히 1건 매칭");
    const auto* foundLuoyang = tableView.GetCityAt(0);
    TEST_ASSERT(foundLuoyang != nullptr && foundLuoyang->id == 23 && foundLuoyang->name.find("낙양") != std::string::npos,
                "검색 결과 낙양 (ID 23) 일치 확인");

    // 4-2. "장" 부분 일치 검색 (장안, 장사 등)
    tableView.SetSearchQuery("장");
    size_t countJang = tableView.GetFilteredCount();
    std::cout << "         -> '장' 검색 결과 수: " << countJang << "개\n";
    TEST_ASSERT(countJang >= 2, "'장' 검색 결과 2건 이상 부분 매칭 확인");

    // 4-3. 숫자 ID 검색 ("23")
    tableView.SetSearchQuery("23");
    TEST_ASSERT(tableView.GetFilteredCount() >= 1, "숫자 ID '23' 검색 결과 포함 확인");

    // 4-4. 미존재 검색어 테스트
    tableView.SetSearchQuery("존재하지않는도시XYZ99");
    TEST_ASSERT(tableView.GetFilteredCount() == 0, "존재하지 않는 도시 검색 결과 0건 확인");

    // 검색어 초기화
    tableView.SetSearchQuery("");
    TEST_ASSERT(tableView.GetFilteredCount() == 60, "검색어 초기화 후 60개 도시 목록 전원 복구");

    // 5. 도시 상세 편집 모달 호출 및 버퍼 상태 검증 (낙양 ID 23)
    std::cout << "\n[Step 5] 도시 상세 편집 모달(OpenEditModal) 호출 및 버퍼 검증 (낙양 ID 23)\n";
    tableView.OpenEditModal(23);
    TEST_ASSERT(tableView.IsEditModalOpen(), "낙양(ID 23) 편집 모달 활성화 확인");
    TEST_ASSERT(tableView.GetEditingCityId() == 23, "편집 대상 ID 23 일치 확인");
    std::cout << "         -> 낙양 현재 자금: " << tableView.GetEditRecord().gold
              << ", 군량: " << tableView.GetEditRecord().provisions
              << ", 내구: " << tableView.GetEditRecord().durability << "\n";

    // 6. SafeVanilla 모드 실시간 핫싱크 및 Bounds 클램핑 검증
    std::cout << "\n[Step 6] SafeVanilla 모드 실시간 핫싱크 및 클램핑 검증\n";
    tableView.SetLimitBreakMode(false);
    // 자금 200만 입력 시 Safe 상한인 999,999로 자동 클램핑 확인
    tableView.GetEditRecord().gold = 2000000;
    bool syncOk = tableView.CommitEditToMemory();
    TEST_ASSERT(syncOk, "실시간 핫싱크 메모리 커밋 성공");
    std::cout << "         -> 핫싱크 소요 시간: " << tableView.GetLastSyncTimeMs() << "ms\n";
    TEST_ASSERT(tableView.GetLastSyncTimeMs() < 50.0f, "핫싱크 지연시간 50ms 미만 (초고속 실시간 반영)");

    CityRecord verifyCity;
    cityMgr.LoadCity(23, verifyCity);
    TEST_ASSERT(verifyCity.gold == 999999, "Safe 모드: 200만 입력 시 인게임 메모리에 999,999로 클램핑 확인");

    // 7. LimitBreak 모드 및 원클릭 치트 검증
    std::cout << "\n[Step 7] LimitBreak 모드 및 빠른 치트(Quick Cheats) 검증\n";
    // LimitBreak 모드: 150만 허용
    tableView.SetLimitBreakMode(true);
    tableView.GetEditRecord().gold = 1500000;
    tableView.CommitEditToMemory();
    cityMgr.LoadCity(23, verifyCity);
    TEST_ASSERT(verifyCity.gold == 1500000, "LimitBreak 모드: 150만 자금 실시간 메모리 패치 성공");

    // 자원 고갈 치트 (공략용)
    tableView.QuickCheatDepleteResources(23);
    cityMgr.LoadCity(23, verifyCity);
    TEST_ASSERT(verifyCity.gold == 0 && verifyCity.provisions == 0, "낙양 자원 고갈 치트 (자금 0, 군량 0) 확인");

    // 자원 풀충전 치트
    tableView.QuickCheatMaxResources(23);
    cityMgr.LoadCity(23, verifyCity);
    TEST_ASSERT(verifyCity.gold == 999999 && verifyCity.provisions == 999999,
                "낙양 자원 풀충전 치트 (자금/군량 999,999) 확인");

    // 8. 데이터 안전 원상 복구 (낙양: 자금 4,000, 군량 44,400, 병력 22,200)
    std::cout << "\n[Step 8] 검증 후 데이터 안전 원상 복구\n";
    tableView.SetLimitBreakMode(false);
    tableView.GetEditRecord().gold = 4000;
    tableView.GetEditRecord().provisions = 44400;
    tableView.GetEditRecord().troops = 22200;
    tableView.CommitEditToMemory();

    cityMgr.LoadCity(23, verifyCity);
    TEST_ASSERT(verifyCity.gold == 4000 && verifyCity.provisions == 44400 &&
                verifyCity.troops == 22200,
                "낙양 원래 자원 수치 완벽 복구 완료 (자금 4,000 / 군량 44,400 / 병력 22,200)");

    // 9. GUI 파이프라인 및 도시 테이블 120프레임 렌더링 검증
    std::cout << "\n[Step 9] Dear ImGui 도시 테이블 및 편집 모달 렌더링 무결성 검증\n";
    WindowConfig config;
    config.title = "삼국지 13 PK 도시 자원 에디터 [Phase 5.4 검증]";
    config.width = 1280;
    config.height = 800;
    config.fontSize = 18.0f;
    config.vsync = true;

    UIManager ui(config);
    if (!ui.Initialize()) {
        std::cerr << "[-] Error: GUI 렌더러 초기화 실패!\n";
        return 1;
    }

    tableView.OpenEditModal(18);

    int frameLimit = smokeTest ? 120 : 600;
    int renderedFrames = 0;
    std::cout << "         -> 렌더 루프 시작 (" << (smokeTest ? "스모크 테스트 120프레임" : "테스트 렌더링") << ")\n";

    while (!ui.ShouldClose() && renderedFrames < frameLimit) {
        ui.BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1260, 780), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("삼국지 13 PK 도시 자원 테이블 & 편집기 [Phase 5.4]", nullptr)) {
            tableView.Render();
        }
        ImGui::End();

        ui.EndFrame();
        renderedFrames++;
    }

    ui.Shutdown();
    TEST_ASSERT(renderedFrames == frameLimit, "ImGui 도시 테이블 및 모달 120프레임 크래시 없이 정상 렌더링 완료");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 60개 도시 테이블 뷰, 자원 핫싱크, 치트 모든 항목 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 5.4 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";

    return 0;
}
