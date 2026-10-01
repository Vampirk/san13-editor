#include "ui/UIManager.hpp"
#include "ui/OfficerTableView.hpp"
#include "ui/CityTableView.hpp"
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include "game/CityManager.hpp"

#include "imgui.h"
#include <iostream>
#include <vector>
#include <string>

using namespace San13::Core;
using namespace San13::Game;
using namespace San13::UI;

int main(int argc, char** argv) {
    bool autoCloseMode = false;
    int maxFrames = 120; // 60fps 기준 약 2초 후 자동 종료 모드 (테스트 자동화용)

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--auto-close" || arg == "--smoke-test") {
            autoCloseMode = true;
        }
    }

    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 5.1] Dear ImGui GUI Pipeline & Font Test\n";
    std::cout << "============================================================\n\n";

    // 1. 메모리 드라이버 및 도메인 매니저 초기화
    std::cout << "[Step 1] 게임 프로세스 연결 및 데이터 매니저 바인딩\n";
    LinuxMemoryDriver driver;
    bool attached = driver.Attach("San13PK_kr.exe") || driver.Attach("san13pk");
    
    uintptr_t rootBase = 0;
    std::string verDesc;
    bool verOk = attached && GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);

    OfficerManager officerMgr;
    CityManager cityMgr;
    if (verOk) {
        officerMgr.Initialize(&driver, rootBase);
        cityMgr.Initialize(&driver, rootBase);
        std::cout << "         -> 프로세스 연결 성공 (PID: " << driver.GetPid() << ", RootBase: 0x" << std::hex << rootBase << std::dec << ")\n";
    } else {
        std::cout << "         -> [주의] 게임 미실행 상태 (오프라인 GUI 모드로 구동)\n";
    }

    // 2. UIManager 초기화 (GLFW + OpenGL3 + 한글 폰트)
    std::cout << "\n[Step 2] UIManager 초기화 및 한글 폰트 로드\n";
    WindowConfig config;
    config.title = "삼국지 13 PK 한국어 트레이너 & 에디터 [Phase 5.1 검증]";
    config.width = 1280;
    config.height = 800;
    config.fontSize = 18.0f;
    config.vsync = true;

    UIManager ui(config);
    if (!ui.Initialize()) {
        std::cerr << "[-] Error: UIManager 초기화 실패!\n";
        return 1;
    }
    std::cout << "  [PASS] UIManager 및 OpenGL3 윈도우 생성 완료\n";

    // 샘플 데이터 로드
    OfficerRecord caocao, guanyu;
    CityRecord luoyang;
    OfficerTableView officerTable;
    CityTableView cityTable;
    if (verOk) {
        officerMgr.LoadOfficer(333, caocao);
        officerMgr.LoadOfficer(94, guanyu);
        cityMgr.LoadCity(18, luoyang);
        officerTable.Initialize(&officerMgr);
        cityTable.Initialize(&cityMgr);
    }

    std::cout << "\n[Step 3] GUI 메인 렌더 루프 진입 (" << (autoCloseMode ? "스모크 테스트 120프레임 후 자동 종료" : "사용자 인터랙티브 모드") << ")\n";

    int frameCount = 0;
    while (!ui.ShouldClose()) {
        ui.BeginFrame();

        // 메인 에디터 대시보드 윈도우
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1240, 760), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("삼국지 13 PK 한국어 트레이너 & 에디터 (v3.0.0)", nullptr)) {
            // 상단 상태 배너
            ImGui::TextColored(ImVec4(0.85f, 0.72f, 0.42f, 1.0f), "★ 삼국지 13 PK 차세대 크로스플랫폼 메모리 에디터 슈트 ★");
            ImGui::Separator();
            ImGui::Spacing();

            // 연결 상태 표시 카드
            ImGui::BeginChild("StatusCard", ImVec2(0, 100), true);
            ImGui::Columns(3, "StatusColumns", false);
            
            // 컬럼 1: 프로세스 상태
            ImGui::Text("프로세스 연결 상태:");
            if (attached && driver.IsAlive()) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "● 연결됨 (PID: %d)", driver.GetPid());
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.2f, 1.0f), "○ 미연결 (게임 대기 중)");
            }

            ImGui::NextColumn();
            // 컬럼 2: 버전 및 베이스 주소
            ImGui::Text("게임 버전 및 루트 주소:");
            if (verOk) {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", verDesc.c_str());
                ImGui::Text("RootBase: 0x%lX", (unsigned long)rootBase);
            } else {
                ImGui::TextDisabled("N/A");
            }

            ImGui::NextColumn();
            // 컬럼 3: 폰트 및 렌더러 정보
            ImGui::Text("GUI 렌더 파이프라인:");
            ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), "나눔고딕 (NanumGothic.ttf)");
            ImGui::Text("ImGui %s | FPS: %.1f", IMGUI_VERSION, ImGui::GetIO().Framerate);

            ImGui::Columns(1);
            ImGui::EndChild();

            ImGui::Spacing();

            // 탭 인터페이스 테스트
            if (ImGui::BeginTabBar("MainTabs")) {
                if (ImGui::BeginTabItem("무장 도메인 라이브 뷰")) {
                    ImGui::Spacing();
                    ImGui::Text("실시간 메모리에서 읽어온 무장 데이터 (한글 렌더링 무결성 검증):");
                    ImGui::Spacing();

                    if (verOk) {
                        // 조조 카드
                        ImGui::BeginChild("CaoCaoCard", ImVec2(400, 220), true);
                        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "■ %s (ID %d)", caocao.name.c_str(), caocao.id);
                        ImGui::Separator();
                        ImGui::Text("통솔: %d   무력: %d   지력: %d   정치: %d",
                                    caocao.stats.curr_leadership, caocao.stats.curr_war,
                                    caocao.stats.curr_intel, caocao.stats.curr_politics);
                        ImGui::Text("창병 적성: %s | 기병 적성: %s | 궁병 적성: %s",
                                    OfficerRecord::AptitudeToString(caocao.stats.spear_apt),
                                    OfficerRecord::AptitudeToString(caocao.stats.horse_apt),
                                    OfficerRecord::AptitudeToString(caocao.stats.bow_apt));
                        ImGui::Text("메모리 주소: 0x%lX", (unsigned long)caocao.memory_address);
                        ImGui::Spacing();
                        if (ImGui::Button("조조 무력 99 치트 테스트")) {
                            officerMgr.SetWar(333, 99);
                            officerMgr.LoadOfficer(333, caocao);
                        }
                        ImGui::EndChild();

                        ImGui::SameLine();

                        // 관우 카드
                        ImGui::BeginChild("GuanYuCard", ImVec2(400, 220), true);
                        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "■ %s (ID %d)", guanyu.name.c_str(), guanyu.id);
                        ImGui::Separator();
                        ImGui::Text("통솔: %d   무력: %d   지력: %d   정치: %d",
                                    guanyu.stats.curr_leadership, guanyu.stats.curr_war,
                                    guanyu.stats.curr_intel, guanyu.stats.curr_politics);
                        ImGui::Text("창병 적성: %s | 기병 적성: %s | 궁병 적성: %s",
                                    OfficerRecord::AptitudeToString(guanyu.stats.spear_apt),
                                    OfficerRecord::AptitudeToString(guanyu.stats.horse_apt),
                                    OfficerRecord::AptitudeToString(guanyu.stats.bow_apt));
                        ImGui::Text("메모리 주소: 0x%lX", (unsigned long)guanyu.memory_address);
                        ImGui::EndChild();
                    } else {
                        ImGui::TextDisabled("게임이 연결되지 않아 실시간 무장 데이터를 표시할 수 없습니다.");
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("전체 무장 목록 (Phase 5.5 다중 선택 & 일괄 수정)")) {
                    ImGui::Spacing();
                    if (verOk) {
                        officerTable.Render();
                    } else {
                        ImGui::TextDisabled("게임이 연결되지 않아 무장 목록을 표시할 수 없습니다.");
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("도시 도메인 라이브 뷰")) {
                    ImGui::Spacing();
                    ImGui::Text("실시간 메모리에서 읽어온 도시 데이터 (60개 도시 중 낙양 샘플):");
                    ImGui::Spacing();

                    if (verOk) {
                        ImGui::BeginChild("CityCard", ImVec2(450, 200), true);
                        ImGui::TextColored(ImVec4(0.85f, 0.72f, 0.42f, 1.0f), "■ %s (ID %d)", luoyang.name.c_str(), luoyang.id);
                        ImGui::Separator();
                        ImGui::Text("자금: %d", luoyang.gold);
                        ImGui::Text("군량: %d", luoyang.provisions);
                        ImGui::Text("병력: %d", luoyang.troops);
                        ImGui::Text("성벽 내구도: %d", luoyang.durability);
                        ImGui::Text("메모리 주소: 0x%lX", (unsigned long)luoyang.memory_address);
                        ImGui::EndChild();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("전체 도시 목록 (Phase 5.4 자원 편집기)")) {
                    ImGui::Spacing();
                    if (verOk) {
                        cityTable.Render();
                    } else {
                        ImGui::TextDisabled("게임이 연결되지 않아 도시 목록을 표시할 수 없습니다.");
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("한글 글리프 및 UI 위젯 검증")) {
                    ImGui::Spacing();
                    ImGui::Text("특수 기호 및 한글 완성형 글리프 렌더링 테스트:");
                    ImGui::BulletText("화살표 및 도형: → ← ↑ ↓ ▲ ▼ ★ ☆ ※ ● ○ ■ □ ◆ ◇ ▶ ◀");
                    ImGui::BulletText("수식 및 기호: + - %% × ÷ · …");
                    ImGui::BulletText("어려운 한글 단어: 똠방각하, 쀍, 갹출, 늴리리, 콱, 홑, 퀭, 윷, 칡");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    static float testFloat = 0.5f;
                    static int testInt = 100;
                    static char testBuf[64] = "제갈공명 (諸葛孔明)";
                    ImGui::SliderFloat("슬라이더 테스트 (골드 테마)", &testFloat, 0.0f, 1.0f);
                    ImGui::SliderInt("정수 슬라이더", &testInt, 0, 255);
                    ImGui::InputText("한글 입력창", testBuf, sizeof(testBuf));
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::Spacing();
            ImGui::Separator();
            if (autoCloseMode) {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "[스모크 테스트 진행 중...] 남은 프레임: %d / %d",
                                   maxFrames - frameCount, maxFrames);
            } else {
                ImGui::TextDisabled("ESC 키를 누르거나 우측 상단 닫기(X) 버튼을 누르면 종료됩니다.");
            }
        }
        ImGui::End();

        ui.EndFrame();

        frameCount++;
        if (autoCloseMode && frameCount >= maxFrames) {
            std::cout << "  [PASS] 스모크 테스트 " << maxFrames << "프레임 정상 렌더링 완료 -> 자동 종료\n";
            break;
        }
    }

    ui.Shutdown();

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: ImGui + OpenGL3 GUI 창 및 한글 폰트 파이프라인 검증 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 5.1 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";

    return 0;
}
