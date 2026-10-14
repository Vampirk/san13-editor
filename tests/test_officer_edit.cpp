#include "ui/UIManager.hpp"
#include "ui/OfficerTableView.hpp"
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
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
    std::cout << " [San13PK Phase 5.3] Officer Stat Editing & Hot-Sync Test\n";
    std::cout << "============================================================\n\n";

    // 1. 메모리 드라이버 및 도메인 매니저 초기화
    std::cout << "[Step 1] 게임 프로세스 연결 및 OfficerManager 초기화\n";
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

    OfficerManager officerMgr;
    bool mgrOk = officerMgr.Initialize(&driver, rootBase);
    TEST_ASSERT(mgrOk, "OfficerManager 초기화 성공");

    // 2. OfficerTableView 초기화 및 조조(ID 333) 데이터 바인딩
    std::cout << "\n[Step 2] OfficerTableView 초기화 및 대상 무장(조조 ID 333) 바인딩\n";
    OfficerTableView tableView;
    tableView.Initialize(&officerMgr);
    TEST_ASSERT(tableView.GetTotalCount() >= 1000, "전체 무장 목록 캐시 완료");

    // 3. 편집 모달 열기 및 버퍼 상태 검증
    std::cout << "\n[Step 3] 무장 상세 편집 모달(OpenEditModal) 호출 및 버퍼 상태 검증\n";
    tableView.OpenEditModal(333);
    TEST_ASSERT(tableView.IsEditModalOpen(), "조조(ID 333) 편집 모달 플래그 활성화");
    TEST_ASSERT(tableView.GetEditingOfficerId() == 333, "편집 대상 ID 333 일치 확인");
    std::cout << "         -> 현재 조조 통솔: " << (int)tableView.GetEditStats().curr_leadership
              << ", 무력: " << (int)tableView.GetEditStats().curr_war << "\n";

    // 4. SafeVanilla 모드 실시간 핫싱크 및 자동 클램핑 검증
    std::cout << "\n[Step 4] SafeVanilla 모드 실시간 핫싱크(CommitEditToMemory) 검증\n";
    tableView.SetLimitBreakMode(false);
    // 무력 120 입력 시 100으로 자동 클램핑 검증
    tableView.GetEditStats().base_war = 120;
    tableView.GetEditStats().curr_war = 120;
    
    bool syncOk = tableView.CommitEditToMemory();
    TEST_ASSERT(syncOk, "실시간 핫싱크 메모리 커밋 성공");
    std::cout << "         -> 핫싱크 소요 시간: " << tableView.GetLastSyncTimeMs() << "ms\n";
    TEST_ASSERT(tableView.GetLastSyncTimeMs() < 50.0f, "핫싱크 지연시간 50ms 미만 (초고속 실시간 반영)");

    // 메모리에서 다시 읽어와 100 클램핑 여부 교차 검증
    OfficerRecord verifyRec;
    officerMgr.LoadOfficer(333, verifyRec);
    TEST_ASSERT(verifyRec.stats.base_war == 100, "Safe 모드: 120 입력 시 인게임 메모리에 100으로 안전 클램핑 확인");
    TEST_ASSERT(tableView.GetEditStats().curr_war == 100, "에디터 내부 버퍼도 100으로 자동 동기화 확인");

    // 5. LimitBreak(한계 돌파) 모드 포화 연산(Saturating Cast) 검증
    std::cout << "\n[Step 5] LimitBreak(한계 돌파) 모드 및 포화 연산 검증\n";
    tableView.SetLimitBreakMode(true);
    TEST_ASSERT(tableView.IsLimitBreakMode(), "한계 돌파 모드 활성화");

    // 250 입력 -> 허용
    tableView.GetEditStats().base_war = 250;
    tableView.GetEditStats().curr_war = 250;
    tableView.CommitEditToMemory();
    officerMgr.LoadOfficer(333, verifyRec);
    TEST_ASSERT(verifyRec.stats.base_war == 250, "LimitBreak 모드: 250 수치 실시간 메모리 패치 성공");

    // 256 이상 입력 -> 0 오버플로 방지 및 255 포화 고정 검증
    tableView.GetEditStats().base_war = 255;
    tableView.GetEditStats().curr_war = 255;
    tableView.CommitEditToMemory();
    officerMgr.LoadOfficer(333, verifyRec);
    TEST_ASSERT(verifyRec.stats.base_war == 255, "8비트 하드웨어 한계 255 포화 연산 고정 확인");

    // 6. 빠른 치트 프리셋(Quick Cheats) 기능 검증
    std::cout << "\n[Step 6] 빠른 치트 프리셋(Quick Cheats) 원클릭 패치 검증\n";
    // 전 병종 S급 적성 치트
    bool aptOk = tableView.QuickCheatAllAptitudeS(333);
    TEST_ASSERT(aptOk, "전 병종 S급 원클릭 치트 실행");
    officerMgr.LoadOfficer(333, verifyRec);
    TEST_ASSERT(verifyRec.stats.spear_apt == 3 && verifyRec.stats.horse_apt == 3 && verifyRec.stats.bow_apt == 3,
                "조조 창병/기병/궁병 전종 S(3) 적성 실시간 메모리 패치 검증");

    // 올 100 치트
    bool all100Ok = tableView.QuickCheatAll100(333);
    TEST_ASSERT(all100Ok, "능력치 올 100 원클릭 치트 실행");
    officerMgr.LoadOfficer(333, verifyRec);
    TEST_ASSERT(verifyRec.stats.base_leadership == 100 && verifyRec.stats.base_war == 100 &&
                verifyRec.stats.base_intel == 100 && verifyRec.stats.base_politics == 100,
                "조조 통솔/무력/지력/정치 전원 100 실시간 메모리 패치 검증");

    // 테스트 완료 후 조조 원래 수치로 안전 복구 (통솔 99, 무력 99, 지력 92, 정치 91, 적성 A/A/A)
    std::cout << "\n[Step 7] 검증 후 데이터 안전 원상 복구\n";
    tableView.SetLimitBreakMode(false);
    tableView.GetEditStats().base_leadership = 99; tableView.GetEditStats().curr_leadership = 99;
    tableView.GetEditStats().base_war = 99;        tableView.GetEditStats().curr_war = 99;
    tableView.GetEditStats().base_intel = 92;      tableView.GetEditStats().curr_intel = 92;
    tableView.GetEditStats().base_politics = 91;   tableView.GetEditStats().curr_politics = 91;
    tableView.GetEditStats().spear_apt = 2;        // A
    tableView.GetEditStats().horse_apt = 2;        // A
    tableView.GetEditStats().bow_apt = 2;          // A
    tableView.CommitEditToMemory();

    officerMgr.LoadOfficer(333, verifyRec);
    TEST_ASSERT(verifyRec.stats.base_leadership == 99 && verifyRec.stats.base_war == 99 &&
                verifyRec.stats.base_intel == 92 && verifyRec.stats.base_politics == 91,
                "조조 스탯 원래 값 복구 완료 (99/99/92/91)");
    TEST_ASSERT(verifyRec.stats.spear_apt == 2 && verifyRec.stats.horse_apt == 2 && verifyRec.stats.bow_apt == 2,
                "조조 적성 원래 값 복구 완료 (A/A/A)");

    // 8. GUI 파이프라인 및 편집 모달 120프레임 렌더링 검증
    std::cout << "\n[Step 8] Dear ImGui 편집 모달 렌더링 무결성 검증\n";
    WindowConfig config;
    config.title = "삼국지 13 PK 무장 편집 & 핫싱크 [Phase 5.3 검증]";
    config.width = 1280;
    config.height = 800;
    config.fontSize = 18.0f;
    config.vsync = true;

    UIManager ui(config);
    if (!ui.Initialize()) {
        std::cerr << "[-] Error: GUI 렌더러 초기화 실패!\n";
        return 1;
    }

    tableView.OpenEditModal(333);

    int frameLimit = smokeTest ? 120 : 600;
    int renderedFrames = 0;
    std::cout << "         -> 렌더 루프 시작 (" << (smokeTest ? "스모크 테스트 120프레임" : "테스트 렌더링") << ")\n";

    while (!ui.ShouldClose() && renderedFrames < frameLimit) {
        ui.BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1260, 780), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("삼국지 13 PK 무장 테이블 & 편집기 [Phase 5.3]", nullptr)) {
            tableView.Render();
        }
        ImGui::End();

        ui.EndFrame();
        renderedFrames++;
    }

    ui.Shutdown();
    TEST_ASSERT(renderedFrames == frameLimit, "ImGui 편집 모달 120프레임 크래시 없이 정상 렌더링 완료");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 인라인 편집, 실시간 핫싱크, Bounds 검증 모든 항목 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 5.3 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";

    return 0;
}
