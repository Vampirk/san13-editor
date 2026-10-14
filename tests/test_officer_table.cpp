#include "ui/UIManager.hpp"
#include "ui/OfficerTableView.hpp"
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include "game/OfficerNameTable.hpp"
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
    std::cout << " [San13PK Phase 5.2] Officer Table View & Slot Expansion Test\n";
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

    uint16_t gameYear = 0, gameMonth = 0, gameDay = 0;
    bool dateOk = officerMgr.GetGameDate(gameYear, gameMonth, gameDay);
    TEST_ASSERT(dateOk, "시나리오 게임 일자 판독 성공");
    std::cout << "         -> 판독된 인게임 일자: " << gameYear << "년 " << gameMonth << "월 " << gameDay << "일\n";
    TEST_ASSERT(gameYear >= 184 && gameYear <= 400 && gameMonth >= 1 && gameMonth <= 12 && gameDay >= 1 && gameDay <= 31,
                "게임 일자 범위 정합성 검증 성공 (세이브 무관, TECH-05)");

    // 2. OfficerTableView 데이터 바인딩 및 동적 슬롯 확장 로드 검증
    std::cout << "\n[Step 2] 800명 역사 무장 + 285+ 확장 슬롯 동적 스캔 로드 검증\n";
    OfficerTableView tableView;
    tableView.Initialize(&officerMgr);

    size_t totalOfficers = tableView.GetTotalCount();
    std::cout << "         -> 총 로드된 무장 레코드 수: " << totalOfficers << "명\n";
    TEST_ASSERT(totalOfficers >= 1000, "1,000명 이상의 무장(역사 800명 + 확장 슬롯) 로드 확인");
    TEST_ASSERT(tableView.GetFilteredCount() == totalOfficers, "초기 전체 필터 카운트 일치 확인");

    // 역사 무장 vs 확장 무장 태그 분기 검증
    const auto* offFirst = tableView.GetOfficerAt(0);
    TEST_ASSERT(offFirst != nullptr && offFirst->id == 0 && !offFirst->is_extended, "0번 무장은 역사 무장 (is_extended == false)");

    bool hasExtended = false;
    for (size_t i = 0; i < tableView.GetTotalCount(); i++) {
        const auto* off = tableView.GetOfficerAt(i);
        if (off && off->id >= 800 && off->is_extended) {
            hasExtended = true;
            std::cout << "         -> 첫 번째 확장 슬롯 발견: ID " << off->id << " (" << off->name 
                      << ") [통솔:" << (int)off->stats.curr_leadership 
                      << ", 무력:" << (int)off->stats.curr_war << "]\n";
            break;
        }
    }
    TEST_ASSERT(hasExtended, "ID 800 이상의 동적 확장 슬롯 레코드 감지 완료");

    // 3. 실시간 다중 컬럼 정렬(Sorting) 알고리즘 검증
    std::cout << "\n[Step 3] 실시간 다중 컬럼 정렬 기능 검증\n";

    // 3-1. 무력(Column 4) 내림차순 정렬 -> 여포 또는 항우가 최상단
    tableView.SortByColumn(4, false);
    const auto* topWar = tableView.GetOfficerAt(0);
    TEST_ASSERT(topWar != nullptr && topWar->stats.curr_war >= 100, 
                "무력 내림차순 정렬: 1위 무력 100 이상 검증 (1위: " + (topWar ? topWar->name : "") + 
                ", 무력: " + std::to_string(topWar ? topWar->stats.curr_war : 0) + ")");

    // 3-2. 지력(Column 5) 내림차순 정렬 -> 제갈량(100) 최상단
    tableView.SortByColumn(5, false);
    const auto* topIntel = tableView.GetOfficerAt(0);
    TEST_ASSERT(topIntel != nullptr && topIntel->stats.curr_intel == 100, 
                "지력 내림차순 정렬: 1위 지력 100 검증 (1위: " + (topIntel ? topIntel->name : "") + ")");

    // 3-3. 총합(Column 7) 내림차순 정렬 -> 조조 / 제갈량 등 최상단
    tableView.SortByColumn(7, false);
    const auto* topTotal = tableView.GetOfficerAt(0);
    TEST_ASSERT(topTotal != nullptr && topTotal->GetTotalStats() >= 350, 
                "스탯 총합 내림차순 정렬: 1위 총합 350 이상 검증 (1위: " + (topTotal ? topTotal->name : "") + 
                ", 총합: " + std::to_string(topTotal ? topTotal->GetTotalStats() : 0) + ")");

    // 3-4. ID(Column 0) 오름차순 복귀 정렬
    tableView.SortByColumn(0, true);
    const auto* topId = tableView.GetOfficerAt(0);
    TEST_ASSERT(topId != nullptr && topId->id == 0, "ID 오름차순 정렬 복귀 검증 (0번 무장 위치)");

    // 4. 실시간 한글 및 ID 검색(Search) 필터 검증
    std::cout << "\n[Step 4] 실시간 한글 및 ID 검색 필터 검증\n";

    // 4-1. "조조" 단일 검색
    tableView.SetSearchQuery("조조");
    TEST_ASSERT(tableView.GetFilteredCount() == 1, "한글 '조조' 검색 결과 정확히 1건 매칭");
    const auto* foundCaoCao = tableView.GetOfficerAt(0);
    TEST_ASSERT(foundCaoCao != nullptr && foundCaoCao->id == 333 && foundCaoCao->name.find("조조") != std::string::npos, 
                "검색 결과 조조 (ID 333) 일치 확인");

    // 4-2. "조" 부분 일치 검색 (조조, 조운 등 복수 매칭)
    tableView.SetSearchQuery("조");
    size_t countJo = tableView.GetFilteredCount();
    std::cout << "         -> '조' 검색 결과 수: " << countJo << "명\n";
    TEST_ASSERT(countJo >= 2, "'조' 검색 결과 2건 이상 (조조, 조운 등) 매칭 확인");

    // 4-3. "관우" 특정 검색
    tableView.SetSearchQuery("관우");
    TEST_ASSERT(tableView.GetFilteredCount() == 1, "'관우' 검색 결과 정확히 1건 매칭");
    const auto* foundGuanYu = tableView.GetOfficerAt(0);
    TEST_ASSERT(foundGuanYu != nullptr && foundGuanYu->id == 94, "검색 결과 관우 (ID 94) 일치 확인");

    // 4-4. 숫자 ID 검색 ("380" -> 조운)
    tableView.SetSearchQuery("380");
    bool found380 = false;
    for (size_t i = 0; i < tableView.GetFilteredCount(); i++) {
        if (tableView.GetOfficerAt(i)->id == 380) {
            found380 = true;
            break;
        }
    }
    TEST_ASSERT(found380, "숫자 ID '380'(조운) 검색 결과 포함 확인");

    // 4-5. 미존재 검색어 테스트
    tableView.SetSearchQuery("존재하지않는특수문자_XYZ_9999");
    TEST_ASSERT(tableView.GetFilteredCount() == 0, "존재하지 않는 검색어 필터링 시 0건 정상 반환");

    // 검색어 초기화
    tableView.SetSearchQuery("");
    TEST_ASSERT(tableView.GetFilteredCount() == totalOfficers, "검색어 리셋 후 전체 무장 목록 복구");

    // 5. 상태별 필터 탭 검증
    std::cout << "\n[Step 5] 상태별 탭 필터(현역/재야/신무장) 검증\n";

    // 5-1. 현역 무장 필터
    tableView.SetFilterMode(OfficerFilterMode::Active);
    size_t activeCount = tableView.GetFilteredCount();
    std::cout << "         -> 현역 무장 수: " << activeCount << "명\n";
    TEST_ASSERT(activeCount > 0 && activeCount < totalOfficers, "현역 무장 필터링 정상 동작");
    bool allActiveValid = true;
    for (size_t i = 0; i < std::min(activeCount, (size_t)50); i++) {
        const auto* off = tableView.GetOfficerAt(i);
        if (off->force_id == 0 || off->force_id == 0xFFFF || off->status_flag == 0xFF) {
            allActiveValid = false;
            break;
        }
    }
    TEST_ASSERT(allActiveValid, "현역 무장 표본 50명 세력 소속 유효성 검증 완료");

    // 5-2. 재야 무장 필터
    tableView.SetFilterMode(OfficerFilterMode::Free);
    size_t freeCount = tableView.GetFilteredCount();
    std::cout << "         -> 재야 무장 수: " << freeCount << "명\n";
    bool allFreeValid = true;
    for (size_t i = 0; i < std::min(freeCount, (size_t)50); i++) {
        const auto* off = tableView.GetOfficerAt(i);
        // [EDGE-32] 재야 무장 조건: status_flag == 0 || force_id == 0 || force_id == 0xFFFF || force_id >= 120
        bool isValidFree = (off->status_flag == 0 || off->force_id == 0 || off->force_id == 0xFFFF || off->force_id >= 120);
        if (!isValidFree) {
            allFreeValid = false;
            break;
        }
    }
    TEST_ASSERT(allFreeValid, "재야 무장 표본 무소속/미사관 유효성 검증 완료 (EDGE-32)");

    // 5-3. 신무장/자녀 무장 (800+) 필터
    tableView.SetFilterMode(OfficerFilterMode::Extended);
    size_t extCount = tableView.GetFilteredCount();
    std::cout << "         -> 확장 슬롯(신무장/자녀) 수: " << extCount << "명\n";
    TEST_ASSERT(extCount > 0, "확장 슬롯 필터링 1건 이상 확인");
    bool allExtValid = true;
    for (size_t i = 0; i < extCount; i++) {
        const auto* off = tableView.GetOfficerAt(i);
        if (!off->is_extended || off->id < 800) {
            allExtValid = false;
            break;
        }
    }
    TEST_ASSERT(allExtValid, "모든 확장 슬롯 무장의 ID 800+ 및 is_extended 플래그 검증 완료");

    // 5-4. [Phase 5] 출신/분류 필터(역사 / 고대 / 신무장) 검증
    std::cout << "         -> 출신/분류 필터(역사/고대/신무장) 검증\n";
    tableView.SetFilterMode(OfficerFilterMode::All);

    // 역사 무장 (ID 0 ~ 699 + PK 추가 사실무장 1151 ~ 1250)
    tableView.SetOriginFilter(OfficerOriginFilter::Historical);
    size_t histCount = tableView.GetFilteredCount();
    std::cout << "         -> 역사 무장(0~699, 1151~1250) 수: " << histCount << "명\n";
    TEST_ASSERT(histCount >= 700 && histCount <= 800, "역사 무장 700~800명 범위 검증");
    bool allHist = true;
    for (size_t i = 0; i < histCount; i++) {
        const auto* off = tableView.GetOfficerAt(i);
        if (!off || !IsHistoricalOfficerId(off->id)) { allHist = false; break; }
    }
    TEST_ASSERT(allHist, "역사 무장 ID 범위(0~699, 1151~1250) 확인 (BUG-01 B-2)");

    // 고대 무장 (ID 850 ~ 899, 1251 ~ 1263)
    tableView.SetOriginFilter(OfficerOriginFilter::Ancient);
    size_t ancCount = tableView.GetFilteredCount();
    std::cout << "         -> 고대 무장(ID 850~899, 1251~1263) 수: " << ancCount << "명\n";
    TEST_ASSERT(ancCount > 0 && ancCount <= 63, "고대 무장 1~63명 감지 확인 (EDGE-56)");
    for (size_t i = 0; i < ancCount; i++) {
        const auto* off = tableView.GetOfficerAt(i);
        TEST_ASSERT(off && IsAncientOfficerId(off->id), "고대 무장 ID 범위(850~899, 1251~1263) 확인");
    }

    // 등록 신무장 (ID 700 ~ 849, 1315 ~ 2164 중 사용 중인 슬롯)
    tableView.SetOriginFilter(OfficerOriginFilter::Created);
    size_t createdCount = tableView.GetFilteredCount();
    std::cout << "         -> 등록 신무장(사용 중 슬롯) 수: " << createdCount << "명\n";
    bool allCreated = true;
    for (size_t i = 0; i < createdCount; i++) {
        const auto* off = tableView.GetOfficerAt(i);
        if (!off || !IsCustomOfficerSlotId(off->id) || (off->status_flag == 0xFF && !HasOfficerKoreanName(off->id))) {
            allCreated = false;
            break;
        }
    }
    TEST_ASSERT(allCreated, "등록 신무장 슬롯 범위 및 미할당 빈 슬롯 제외 확인 (TECH-07)");

    // 출신 필터 리셋
    tableView.SetOriginFilter(OfficerOriginFilter::All);
    tableView.SetFilterMode(OfficerFilterMode::All);
    TEST_ASSERT(tableView.GetFilteredCount() == totalOfficers, "전체 필터 복귀 확인");

    // 6. GUI 파이프라인 및 ImGuiListClipper 가상 스크롤 렌더링 검증
    std::cout << "\n[Step 6] Dear ImGui 가상 스크롤(ImGuiListClipper) 렌더링 무결성 검증\n";
    WindowConfig config;
    config.title = "삼국지 13 PK 무장 테이블 뷰 [Phase 5.2 검증]";
    config.width = 1280;
    config.height = 800;
    config.fontSize = 18.0f;
    config.vsync = true;

    UIManager ui(config);
    if (!ui.Initialize()) {
        std::cerr << "[-] Error: GUI 렌더러 초기화 실패!\n";
        return 1;
    }

    int frameLimit = smokeTest ? 120 : 600;
    int renderedFrames = 0;
    std::cout << "         -> 렌더 루프 시작 (" << (smokeTest ? "스모크 테스트 120프레임" : "테스트 렌더링") << ")\n";

    while (!ui.ShouldClose() && renderedFrames < frameLimit) {
        ui.BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1260, 780), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("삼국지 13 PK 무장 테이블 [Phase 5.2 가상 스크롤 뷰]", nullptr)) {
            tableView.Render();
        }
        ImGui::End();

        ui.EndFrame();
        renderedFrames++;
    }

    ui.Shutdown();
    TEST_ASSERT(renderedFrames == frameLimit, "ImGuiListClipper 120프레임 크래시 및 버퍼 오버런 없이 정상 렌더링 완료");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 가상 스크롤 테이블 뷰 & 동적 슬롯 확장 모든 항목 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 5.2 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";

    return 0;
}
