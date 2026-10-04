#include "ui/UIManager.hpp"
#include "ui/CityDetailModal.hpp"
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/CityManager.hpp"
#include "game/SettlementManager.hpp"
#include "imgui.h"

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
    std::cout << " [San13PK Phase 4] 4-Tab City Inspector & Direct Editing Unit Test\n";
    std::cout << "============================================================\n\n";

    // 1. 메모리 드라이버 및 매니저 초기화
    std::cout << "[Step 1] 게임 프로세스 연결 및 도메인 매니저 초기화\n";
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
    TEST_ASSERT(cityMgr.Initialize(&driver, rootBase), "CityManager 초기화 성공");

    SettlementManager setMgr;
    TEST_ASSERT(setMgr.Initialize(&driver, rootBase), "SettlementManager 초기화 성공");

    // 2. CityDetailModal 초기화 및 낙양(ID 23) 오픈
    std::cout << "\n[Step 2] CityDetailModal 초기화 및 낙양(ID 23) 오픈\n";
    CityDetailModal modal;
    modal.Initialize(&cityMgr, &setMgr);

    TEST_ASSERT(!modal.IsOpen(), "초기 상태 모달 닫힘 확인");
    modal.Open(23);
    TEST_ASSERT(modal.IsOpen(), "낙양(ID 23) 오픈 후 IsOpen true 확인");
    TEST_ASSERT(modal.GetCityId() == 23, "모달 대상 도시 ID 23 일치");

    const auto& rec = modal.GetEditingRecord();
    std::cout << "         -> 대상 도시: " << rec.name << " [ID " << (int)rec.id << "]\n";
    TEST_ASSERT(rec.name.find("낙양") != std::string::npos, "도시 이름 '낙양' 일치 확인");
    TEST_ASSERT(rec.settlement_count == 4, "낙양 관할 부락 수 4개 일치 확인");
    std::cout << "         -> 낙양 소속 무장 수: " << rec.officer_count << "명, 재야: " << rec.free_officer_count << "명\n";
    TEST_ASSERT(rec.durability > 0, "낙양 성벽 내구도 유효 확인");

    // 3. Tab 1: 경제/자원 핫싱크 및 클램핑 검증
    std::cout << "\n[Step 3] Tab 1: 자원 핫싱크 및 자동 복구 검증\n";

    bool commitOk = modal.CommitEditToMemory();
    TEST_ASSERT(commitOk, "메모리 핫싱크 커밋 성공");
    std::cout << "         -> 핫싱크 소요 시간: " << modal.GetLastSyncTimeMs() << "ms\n";
    TEST_ASSERT(modal.GetLastSyncTimeMs() < 50.0f, "핫싱크 50ms 미만 초고속 통과");

    // 4. Tab 2: 내정 및 관할 부락 데이터 로드 검증
    std::cout << "\n[Step 4] Tab 2: 내정 4대치 및 관할 부락 연동 검증\n";
    TEST_ASSERT(rec.public_order > 0, "민심 수치 유효 확인");
    TEST_ASSERT(rec.commerce > 0, "상업 기본치 유효 확인");
    TEST_ASSERT(rec.agriculture > 0, "농업 기본치 유효 확인");
    TEST_ASSERT(rec.culture > 0, "문화 기본치 유효 확인");
    std::cout << "         -> 내정치: 민심=" << rec.public_order
              << ", 상업=" << rec.commerce
              << ", 농업=" << rec.agriculture
              << ", 문화=" << rec.culture << "\n";

    // 5. Tab 3: 병종 3대 숙련도 검증
    std::cout << "\n[Step 5] Tab 3: 병과 3대 숙련도 검증\n";
    TEST_ASSERT(rec.spear_proficiency > 0, "창병 숙련도 유효 확인");
    TEST_ASSERT(rec.horse_proficiency > 0, "기병 숙련도 유효 확인");
    TEST_ASSERT(rec.bow_proficiency > 0, "궁병 숙련도 유효 확인");
    std::cout << "         -> 병과 숙련도: 창=" << rec.spear_proficiency
              << ", 기=" << rec.horse_proficiency
              << ", 궁=" << rec.bow_proficiency << "\n";

    // 6. [Phase 4] 내정, 숙련도, 부락 장악도 직접 편집, 클램핑, 빠른 치트 및 안전 복구 검증
    std::cout << "\n[Step 6] Phase 4: 내정/숙련도/부락 직접 편집, 클램핑 및 안전 복구 검증\n";

    // 6-1. 원본 스냅샷 캡처
    uint16_t origOrder = rec.public_order;
    uint16_t origComm = rec.commerce;
    uint16_t origAgri = rec.agriculture;
    uint16_t origCult = rec.culture;
    uint16_t origSpear = rec.spear_proficiency;
    uint16_t origHorse = rec.horse_proficiency;
    uint16_t origBow = rec.bow_proficiency;
    auto origSettlements = modal.GetSettlements();

    std::cout << "         -> 원본 스냅샷 캡처 완료 (민심=" << origOrder
              << ", 상업=" << origComm << ", 창병숙련=" << origSpear << ")\n";

    // 6-2. 내정 수치 직접 수정 및 메모리 커밋
    modal.GetEditingRecord().public_order = 950;
    modal.GetEditingRecord().commerce = 2500;
    modal.GetEditingRecord().agriculture = 2600;
    modal.GetEditingRecord().culture = 2700;
    TEST_ASSERT(modal.CommitEditToMemory(), "내정 수치 직접 수정 커밋 성공");
    TEST_ASSERT(rec.public_order == 950, "민심 950 인게임 메모리 반영 확인");
    TEST_ASSERT(rec.commerce == 2500, "상업 2,500 인게임 메모리 반영 확인");
    TEST_ASSERT(rec.agriculture == 2600, "농업 2,600 인게임 메모리 반영 확인");
    TEST_ASSERT(rec.culture == 2700, "문화 2,700 인게임 메모리 반영 확인");

    // 6-3. SafeVanilla 클램핑 검증 (민심 9,999 -> 1,000, 상업 50,000 -> 4,000)
    modal.GetEditingRecord().public_order = 9999;
    modal.GetEditingRecord().commerce = 50000;
    modal.CommitEditToMemory();
    TEST_ASSERT(rec.public_order == StatBounds::CityPublicOrder::VANILLA_MAX, "Safe 모드: 민심 1,000 클램핑 확인");
    TEST_ASSERT(rec.commerce == StatBounds::CityDomestic::VANILLA_MAX, "Safe 모드: 상업 4,000 클램핑 확인");

    // 6-4. 빠른 내정 치트 프리셋 검증 (QuickCheatMaxDomestic)
    TEST_ASSERT(modal.QuickCheatMaxDomestic(), "내정 올 3,000 완충 치트 실행 성공");
    TEST_ASSERT(rec.public_order == 1000, "치트 적용 후 민심 1,000 확인");
    TEST_ASSERT(rec.commerce == 3000, "치트 적용 후 상업 3,000 확인");
    TEST_ASSERT(rec.agriculture == 3000, "치트 적용 후 농업 3,000 확인");
    TEST_ASSERT(rec.culture == 3000, "치트 적용 후 문화 3,000 확인");

    // 6-5. 병과 숙련도 직접 수정 및 클램핑 검증
    modal.GetEditingRecord().spear_proficiency = 1500;
    modal.GetEditingRecord().horse_proficiency = 1800;
    modal.GetEditingRecord().bow_proficiency = 2200;
    TEST_ASSERT(modal.CommitEditToMemory(), "병과 숙련도 직접 수정 커밋 성공");
    TEST_ASSERT(rec.spear_proficiency == 1500, "창병 숙련도 1,500 메모리 반영 확인");
    TEST_ASSERT(rec.horse_proficiency == 1800, "기병 숙련도 1,800 메모리 반영 확인");
    TEST_ASSERT(rec.bow_proficiency == 2200, "궁병 숙련도 2,200 메모리 반영 확인");

    // 숙련도 클램핑 (9,999 -> 3,000)
    modal.GetEditingRecord().spear_proficiency = 9999;
    modal.CommitEditToMemory();
    TEST_ASSERT(rec.spear_proficiency == StatBounds::CityProficiency::VANILLA_MAX, "Safe 모드: 창병 숙련도 3,000 클램핑 확인");

    // 6-6. 빠른 숙련도 치트 프리셋 검증 (QuickCheatMaxProficiencies)
    TEST_ASSERT(modal.QuickCheatMaxProficiencies(), "전 병종 숙련도 3,000 완충 치트 실행 성공");
    TEST_ASSERT(rec.spear_proficiency == 3000, "창병 숙련도 3,000 확인");
    TEST_ASSERT(rec.horse_proficiency == 3000, "기병 숙련도 3,000 확인");
    TEST_ASSERT(rec.bow_proficiency == 3000, "궁병 숙련도 3,000 확인");

    // 6-7. 부락 장악도 직접 편집 및 100% 장악 치트 검증
    if (!modal.GetSettlements().empty()) {
        modal.GetSettlements()[0].dev_progress = 50;
        TEST_ASSERT(modal.CommitEditToMemory(), "부락 0번 장악도 50% 수정 커밋 성공");
        TEST_ASSERT(modal.GetSettlements()[0].dev_progress == 50, "부락 0번 장악도 50% 메모리 반영 확인");
    }
    TEST_ASSERT(modal.QuickCheatMaxSettlements(), "관할 부락 100% 일괄 장악 치트 실행 성공");
    for (const auto& s : modal.GetSettlements()) {
        TEST_ASSERT(s.dev_progress == 100, "관할 부락 장악도 100% 완충 확인");
    }

    // 6-8. 원본 데이터 100% 무결성 롤백 (Safe Rollback)
    std::cout << "         -> 검증 완료 후 원본 상태로 롤백 수행\n";
    modal.GetEditingRecord().public_order = origOrder;
    modal.GetEditingRecord().commerce = origComm;
    modal.GetEditingRecord().agriculture = origAgri;
    modal.GetEditingRecord().culture = origCult;
    modal.GetEditingRecord().spear_proficiency = origSpear;
    modal.GetEditingRecord().horse_proficiency = origHorse;
    modal.GetEditingRecord().bow_proficiency = origBow;
    for (size_t i = 0; i < origSettlements.size() && i < modal.GetSettlements().size(); ++i) {
        modal.GetSettlements()[i].dev_progress = origSettlements[i].dev_progress;
    }
    TEST_ASSERT(modal.CommitEditToMemory(), "원본 상태 롤백 커밋 성공");
    TEST_ASSERT(rec.public_order == origOrder, "민심 원본 복구 일치 확인");
    TEST_ASSERT(rec.commerce == origComm, "상업 원본 복구 일치 확인");
    TEST_ASSERT(rec.agriculture == origAgri, "농업 원본 복구 일치 확인");
    TEST_ASSERT(rec.culture == origCult, "문화 원본 복구 일치 확인");
    TEST_ASSERT(rec.spear_proficiency == origSpear, "창병 숙련도 원본 복구 일치 확인");
    TEST_ASSERT(rec.horse_proficiency == origHorse, "기병 숙련도 원본 복구 일치 확인");
    TEST_ASSERT(rec.bow_proficiency == origBow, "궁병 숙련도 원본 복구 일치 확인");
    for (size_t i = 0; i < origSettlements.size() && i < modal.GetSettlements().size(); ++i) {
        TEST_ASSERT(modal.GetSettlements()[i].dev_progress == origSettlements[i].dev_progress,
                    "관할 부락 장악도 원본 복구 일치 확인");
    }
    std::cout << "         -> 낙양 원본 데이터 100% 복구 완료\n";

    // 7. Dear ImGui 4탭 렌더링 무결성 스모크 테스트 (120프레임)
    std::cout << "\n[Step 7] Dear ImGui 4탭 인스펙터 120프레임 무결성 검증 (새 슬라이더/치트 UI 포함)\n";
    WindowConfig config;
    config.title = "삼국지 13 PK 도시 종합 인스펙터 4탭 테스트";
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

        if (ImGui::Begin("도시 종합 인스펙터 테스트 윈도우", nullptr)) {
            modal.Render();
        }
        ImGui::End();

        ui.EndFrame();
        renderedFrames++;
    }

    ui.Shutdown();
    TEST_ASSERT(renderedFrames == frameLimit, "ImGui 4탭 인스펙터 크래시 없이 120프레임 정상 렌더링 완료");

    // 8. 모달 닫기 검증
    std::cout << "\n[Step 8] 모달 닫기 검증\n";
    modal.Close();
    TEST_ASSERT(!modal.IsOpen(), "모달 정상 닫힘 확인");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 4탭 인스펙터, 내정/숙련도/부락 직접 수정, 클램핑,\n";
    std::cout << "            치트 프리셋, 무결성 롤백, 120프레임 렌더 100% 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 4 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";

    return 0;
}
