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
#include <algorithm>

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
    std::cout << " [San13PK Phase 5.5] Batch Actions & Multi-Select Test\n";
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

    // 2. OfficerTableView 바인딩 및 다중 선택 기본 로직 검증
    std::cout << "\n[Step 2] OfficerTableView 초기화 및 다중 선택(Multi-Select) 기본 로직 검증\n";
    OfficerTableView tableView;
    tableView.Initialize(&officerMgr);
    size_t totalCount = tableView.GetTotalCount();
    size_t filteredCount = tableView.GetFilteredCount();
    TEST_ASSERT(totalCount >= 1000, "1,000명 이상 무장 로드 완료");

    // 단일 선택 토글 테스트
    tableView.ClearSelection();
    TEST_ASSERT(tableView.GetSelectedCount() == 0, "초기 선택 목록 비어있음 확인");

    tableView.SelectOfficer(10, true);
    TEST_ASSERT(tableView.IsOfficerSelected(10), "10번 무장 선택 성공");
    TEST_ASSERT(tableView.GetSelectedCount() == 1, "선택 수 1명 확인");

    tableView.ToggleSelectOfficer(10);
    TEST_ASSERT(!tableView.IsOfficerSelected(10), "10번 무장 토글 해제 성공");
    TEST_ASSERT(tableView.GetSelectedCount() == 0, "선택 수 0명 복귀");

    // 전체 선택 / 해제 / 반전 테스트
    tableView.SelectAllFiltered();
    TEST_ASSERT(tableView.GetSelectedCount() == filteredCount, "현재 필터 목록 전체 선택(SelectAll) 확인");

    tableView.ClearSelection();
    TEST_ASSERT(tableView.GetSelectedCount() == 0, "선택 전체 해제(ClearSelection) 확인");

    tableView.InvertSelection();
    TEST_ASSERT(tableView.GetSelectedCount() == filteredCount, "선택 반전(InvertSelection) 확인");

    tableView.ClearSelection();

    // 동일 소재지(도시) 및 신분 기준 일괄 선택 테스트
    tableView.SelectSameCityAs(0);
    size_t cityCount = tableView.GetSelectedCount();
    std::cout << "         -> 0번 무장 소재 도시(City 37) 소속 무장 수: " << cityCount << "명\n";
    TEST_ASSERT(cityCount >= 10, "동일 도시(City 37) 무장 10명 이상 일괄 선택 확인");
    TEST_ASSERT(tableView.IsOfficerSelected(0), "0번 무장 본인 선택 포함 확인");

    tableView.SelectByRank(0); // 군주 (Rank 0)
    size_t rulerCount = tableView.GetSelectedCount();
    std::cout << "         -> 전국 군주(Rank 0) 무장 수: " << rulerCount << "명\n";
    TEST_ASSERT(rulerCount >= 10, "전국 군주(Rank 0) 무장 10명 이상 일괄 선택 확인");
    TEST_ASSERT(tableView.IsOfficerSelected(333), "조조(군주) 선택 포함 확인");

    // 3. 다중 선택 50인 표본 추출 및 원본 스냅샷 캡처
    std::cout << "\n[Step 3] 다중 무장 50인 이상(0~49번 50명) 표본 선택 및 원본 스냅샷 저장\n";
    tableView.ClearSelection();
    std::vector<uint16_t> target50;
    for (uint16_t id = 0; id < 50; id++) {
        tableView.SelectOfficer(id, true);
        target50.push_back(id);
    }
    TEST_ASSERT(tableView.GetSelectedCount() == 50, "정확히 50인 무장 다중 선택 완료");

    std::vector<OfficerSnapshot> originalSnapshots;
    bool snapOk = officerMgr.CreateSnapshots(target50, originalSnapshots);
    TEST_ASSERT(snapOk && originalSnapshots.size() == 50, "50인 무장 원본 상태 스냅샷 100% 캡처 완료");

    // 4. 아군 스탯 일괄 +10 버프 (Batch Stat Delta) 검증
    std::cout << "\n[Step 4] 50인 무장 스탯 일괄 +10 가산 (SafeVanilla 100 상한) 검증\n";
    bool deltaOk = tableView.ApplyBatchStatsDelta(10, 10, 10, 10);
    TEST_ASSERT(deltaOk, "50인 일괄 스탯 +10 트랜잭션 적용 성공");

    float batchTime = tableView.GetLastBatchTimeMs();
    std::cout << "         -> 50인 일괄 패치 소요 시간: " << batchTime << "ms (기준: 50ms 미만)\n";
    TEST_ASSERT(batchTime < 50.0f, "50인 일괄 메모리 패치 50ms 미만 초고속 통과");

    // 인게임 라이브 메모리 직접 읽기 검증
    bool allStatsPlus10 = true;
    for (size_t i = 0; i < originalSnapshots.size(); i++) {
        OfficerRecord rec;
        officerMgr.LoadOfficer(originalSnapshots[i].id, rec);

        int expectedLead = std::min(100, (int)originalSnapshots[i].stats.curr_leadership + 10);
        int expectedWar  = std::min(100, (int)originalSnapshots[i].stats.curr_war + 10);
        int expectedIntel= std::min(100, (int)originalSnapshots[i].stats.curr_intel + 10);
        int expectedPol  = std::min(100, (int)originalSnapshots[i].stats.curr_politics + 10);

        if (rec.stats.curr_leadership != expectedLead ||
            rec.stats.curr_war != expectedWar ||
            rec.stats.curr_intel != expectedIntel ||
            rec.stats.curr_politics != expectedPol) {
            allStatsPlus10 = false;
            std::cerr << "         [Mismatch] ID " << rec.id << " Lead:" << (int)rec.stats.curr_leadership << " vs " << expectedLead << "\n";
            break;
        }
    }
    TEST_ASSERT(allStatsPlus10, "50인 전원 인게임 메모리 통솔/무력/지력/정치 +10 정확히 반영 확인");

    // 5. 수명 일괄 +20년 연장 (Batch Life Extension) 검증
    std::cout << "\n[Step 5] 50인 무장 수명(사망년도) 일괄 +20년 연장 검증\n";
    bool lifeOk = tableView.ApplyBatchLifeExtension(20);
    TEST_ASSERT(lifeOk, "50인 일괄 수명 +20년 연장 트랜잭션 성공");

    bool allLifePlus20 = true;
    for (size_t i = 0; i < originalSnapshots.size(); i++) {
        OfficerRecord rec;
        officerMgr.LoadOfficer(originalSnapshots[i].id, rec);
        int expectedDeath = std::min(400, (int)originalSnapshots[i].death_year + 20);
        if (rec.death_year != expectedDeath) {
            allLifePlus20 = false;
            break;
        }
    }
    TEST_ASSERT(allLifePlus20, "50인 전원 인게임 메모리 사망년도 +20년 연장 일치 확인");

    // 6. 병종 적성 전종 S급 일괄 승급 (Batch Aptitude Promotion) 검증
    std::cout << "\n[Step 6] 50인 무장 병종 적성 전종 S급(창병/기병/궁병 S) 일괄 승급 검증\n";
    bool aptOk = tableView.ApplyBatchAptitudes(3, 3, 3);
    TEST_ASSERT(aptOk, "50인 일괄 전종 S급 적성 트랜잭션 성공");

    bool allRankS = true;
    for (size_t i = 0; i < originalSnapshots.size(); i++) {
        OfficerRecord rec;
        officerMgr.LoadOfficer(originalSnapshots[i].id, rec);
        if (rec.stats.spear_apt != 3 || rec.stats.horse_apt != 3 || rec.stats.bow_apt != 3) {
            allRankS = false;
            break;
        }
    }
    TEST_ASSERT(allRankS, "50인 전원 창병/기병/궁병 S(3) 적성 실시간 패치 확인");

    // 7. 트랜잭션 롤백 (원본 스냅샷 복구) 검증
    std::cout << "\n[Step 7] 트랜잭션 롤백(Undo / Restore) 무결성 검증\n";
    bool restoreOk = officerMgr.BatchRestore(originalSnapshots);
    TEST_ASSERT(restoreOk, "50인 원본 상태 스냅샷 복구(BatchRestore) 성공");
    tableView.RefreshData();

    bool allRestored = true;
    for (size_t i = 0; i < originalSnapshots.size(); i++) {
        OfficerRecord rec;
        officerMgr.LoadOfficer(originalSnapshots[i].id, rec);
        const auto& orig = originalSnapshots[i];

        if (rec.stats.curr_leadership != orig.stats.curr_leadership ||
            rec.stats.curr_war != orig.stats.curr_war ||
            rec.stats.curr_intel != orig.stats.curr_intel ||
            rec.stats.curr_politics != orig.stats.curr_politics ||
            rec.stats.spear_apt != orig.stats.spear_apt ||
            rec.stats.horse_apt != orig.stats.horse_apt ||
            rec.stats.bow_apt != orig.stats.bow_apt ||
            rec.death_year != orig.death_year) {
            allRestored = false;
            std::cerr << "         [Restore Mismatch] ID " << rec.id << "\n";
            break;
        }
    }
    TEST_ASSERT(allRestored, "50인 전원 스탯, 적성, 수명 원본 값으로 100% 무결성 복구 검증");

    // 8. 빠른 일괄 치트 프리셋 (올 100 및 한계돌파 255) 검증
    std::cout << "\n[Step 8] 빠른 일괄 치트 프리셋(올 100 & 한계돌파 255) 및 Undo 검증 (10인 표본)\n";
    tableView.ClearSelection();
    for (uint16_t id = 10; id < 20; id++) {
        tableView.SelectOfficer(id, true);
    }
    TEST_ASSERT(tableView.GetSelectedCount() == 10, "10인 표본 선택 완료");

    // 8-1. 올 100 치트
    bool all100Ok = tableView.BatchCheatAll100();
    TEST_ASSERT(all100Ok, "10인 올 100 일괄 치트 적용 성공");
    for (uint16_t id = 10; id < 20; id++) {
        OfficerRecord rec;
        officerMgr.LoadOfficer(id, rec);
        TEST_ASSERT(rec.stats.curr_leadership == 100 && rec.stats.curr_war == 100 &&
                    rec.stats.curr_intel == 100 && rec.stats.curr_politics == 100,
                    "ID " + std::to_string(id) + " 올 100 일치 확인");
    }

    // 8-2. 직전 작업 Undo 실행
    bool undoOk = tableView.UndoLastBatchOperation();
    TEST_ASSERT(undoOk, "직전 일괄 작업(올 100) 실행 취소(Undo) 성공");

    // 8-3. 한계 돌파 255 치트
    bool all255Ok = tableView.BatchCheatAll255();
    TEST_ASSERT(all255Ok, "10인 한계 돌파 올 255 일괄 치트 적용 성공");
    for (uint16_t id = 10; id < 20; id++) {
        OfficerRecord rec;
        officerMgr.LoadOfficer(id, rec);
        TEST_ASSERT(rec.stats.curr_leadership == 255 && rec.stats.curr_war == 255,
                    "ID " + std::to_string(id) + " 한계 돌파 255 일치 확인");
    }

    // 복구
    tableView.UndoLastBatchOperation();
    std::cout << "         -> 10인 표본 원래 상태로 복구 완료\n";

    // 9. Dear ImGui 다중 선택 및 일괄 모달 렌더링 무결성 스모크 테스트
    std::cout << "\n[Step 9] Dear ImGui 다중 선택 및 일괄 수정 모달 120프레임 렌더링 무결성 검증\n";
    WindowConfig config;
    config.title = "San13PK Phase 5.5 Batch Action Test";
    config.width = 1280;
    config.height = 800;
    config.fontSize = 18.0f;
    config.vsync = true;

    UIManager ui(config);
    bool uiOk = ui.Initialize();
    TEST_ASSERT(uiOk, "UIManager 초기화 성공");

    // 모달 활성화
    tableView.OpenBatchModal();
    TEST_ASSERT(tableView.IsBatchModalOpen(), "일괄 수정 모달(OpenBatchModal) 활성화 확인");

    int frameLimit = smokeTest ? 120 : 600;
    int frames = 0;
    while (!ui.ShouldClose() && frames < frameLimit) {
        ui.BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1260, 780), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Phase 5.5 일괄 수정 테스트 윈도우", nullptr)) {
            tableView.Render();
            ImGui::End();
        }

        ui.EndFrame();
        frames++;
    }
    ui.Shutdown();
    TEST_ASSERT(frames == frameLimit, "ImGui 다중 선택 테이블 및 일괄 모달 크래시 없이 정상 렌더링 완료");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 다중 선택, 50인 스탯 가산, 수명 연장, S급 승급,\n";
    std::cout << "            트랜잭션 롤백, 빠른 프리셋 모든 항목 100% 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> [PHASE 5.5 ACCEPTANCE CRITERIA 100% 통과] <<<\n\n";

    return 0;
}
