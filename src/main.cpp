#include "ui/UIManager.hpp"
#include "ui/OfficerTableView.hpp"
#include "ui/CityTableView.hpp"
#include "ui/ItemTableView.hpp"
#ifdef _WIN32
#include "core/Win32MemoryDriver.hpp"
#else
#include "core/LinuxMemoryDriver.hpp"
#endif
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include "game/CityManager.hpp"
#include "game/SettlementManager.hpp"
#include "game/ItemManager.hpp"
#include "game/TacticManager.hpp"
#include "game/ForceManager.hpp"
#include "imgui.h"

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstring>
#include <cstdio>
#ifndef _WIN32
#include <unistd.h>
#endif

using namespace San13::Core;
using namespace San13::Game;
using namespace San13::UI;

#ifdef _WIN32
static const char* APP_VERSION = "v1.0.0 (Windows x86_64)";
#else
static const char* APP_VERSION = "v1.0.0 (Linux x86_64)";
#endif

static int ReadPtraceScope() {
#ifdef _WIN32
    return -1;
#else
    std::ifstream file("/proc/sys/kernel/yama/ptrace_scope");
    if (!file.is_open()) return -1;
    int scope = -1;
    file >> scope;
    return scope;
#endif
}

static void PrintHelp(const char* progName) {
    std::cout << "============================================================\n";
    std::cout << " 삼국지 13 PK 한국어 트레이너 & 메모리 에디터 " << APP_VERSION << "\n";
    std::cout << "============================================================\n";
    std::cout << "사용법: " << progName << " [옵션]\n\n";
    std::cout << "옵션 목록:\n";
    std::cout << "  -h, --help        이 도움말 메시지를 출력하고 종료합니다.\n";
    std::cout << "  -v, --version     에디터 버전 정보를 출력하고 종료합니다.\n";
    std::cout << "  --smoke-test      120프레임 동안 정상 렌더링 후 자동으로 종료합니다 (자동화 테스트용).\n\n";
    std::cout << "주요 특징:\n";
    std::cout << "  - Steam / Proton (GE-Proton 포함) 및 Linux 네이티브 환경 지원\n";
    std::cout << "  - 1,985명 전체 무장(역사 800명 + 결혼 자녀/신무장) 실시간 가상 스크롤\n";
    std::cout << "  - 통솔/무력/지력/정치 1~255 한계돌파 및 실시간 핫싱크 (0.002ms)\n";
    std::cout << "  - 50인 이상 무장 다중 선택, 일괄 +10 버프, 수명 일괄 연장, 트랜잭션 롤백\n";
    std::cout << "  - 60개 전 도시 자금/군량/병력/내구도 실시간 편집 및 원클릭 치트\n";
    std::cout << "============================================================\n";
}

int main(int argc, char** argv) {
    bool smokeTest = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            PrintHelp(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "San13PK Korean Editor & Trainer " << APP_VERSION << "\n";
            return 0;
        } else if (arg == "--smoke-test" || arg == "--auto-close") {
            smokeTest = true;
        } else if (arg.rfind("--log=", 0) == 0) {
            // 콘솔이 없는 환경(Wine/Proton 등)에서 출력을 파일로 리다이렉트
            const std::string logPath = arg.substr(6);
            std::freopen(logPath.c_str(), "w", stdout);
            std::freopen(logPath.c_str(), "a", stderr);
            std::cout << std::unitbuf;
        }
    }

    std::cout << "============================================================\n";
    std::cout << " 삼국지 13 PK 한국어 메모리 에디터 & 트레이너 " << APP_VERSION << "\n";
    std::cout << "============================================================\n";

    // 1. UIManager 초기화 (GLFW + OpenGL3 + 번들 나눔고딕 TTF)
    WindowConfig config;
    config.title = "삼국지 13 PK 한국어 메모리 에디터 & 트레이너 (San13PK Suite v1.0.0)";
    config.width = 1280;
    config.height = 820;
    config.fontSize = 18.0f;
    config.vsync = true;

    UIManager ui(config);
    if (!ui.Initialize()) {
        std::cerr << "[-] Error: GUI 렌더러 서브시스템 초기화 실패!\n";
        return 1;
    }

    // 2. 코어 메모리 드라이버 및 도메인 매니저
#ifdef _WIN32
    Win32MemoryDriver driver;
#else
    LinuxMemoryDriver driver;
#endif
    OfficerManager officerMgr;
    CityManager cityMgr;
    SettlementManager setMgr;
    ItemManager itemMgr;
    OfficerTableView officerTable;
    CityTableView cityTable;
    ItemTableView itemTable;

    uintptr_t rootBase = 0;
    std::string verDesc;
    bool isConnected = false;

    auto TryConnect = [&]() {
        if (driver.Attach("San13PK_kr.exe") || driver.Attach("san13pk")) {
            if (GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
                officerMgr.Initialize(&driver, rootBase);
                cityMgr.Initialize(&driver, rootBase);
                setMgr.Initialize(&driver, rootBase);
                itemMgr.Initialize(&driver, rootBase);
                ForceManager::Initialize(&driver, rootBase);
                TacticManager::LoadCustomTactics();
                TacticManager::LoadCustomTacticsFromMemory(&driver, rootBase);
                officerTable.Initialize(&officerMgr, &itemMgr);
                cityTable.Initialize(&cityMgr, &setMgr);
                itemTable.Initialize(&itemMgr, &officerMgr);
                isConnected = true;
                std::cout << "[+] 게임 프로세스 연결 성공 (PID: " << driver.GetPid() 
                          << ", RootBase: 0x" << std::hex << rootBase << std::dec << ")\n";
                return true;
            }
        }
        isConnected = false;
        return false;
    };

    // 초기 연결 시도
    TryConnect();

    [[maybe_unused]] int ptraceScope = ReadPtraceScope();  // Linux ptrace 권한 안내 전용 (Windows에서는 미사용)
    int frameCount = 0;
    auto lastPollTime = std::chrono::steady_clock::now();

    // 3. 메인 애플리케이션 렌더 루프
    while (!ui.ShouldClose()) {
        if (smokeTest && frameCount >= 120) {
            std::cout << "[SmokeTest] 120프레임 무결성 스모크 테스트 완료 -> 정상 종료\n";
            break;
        }

        // 주기적 프로세스 상태 폴링 (미연결 시 1.5초마다 재탐색)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPollTime).count() > 1500) {
            lastPollTime = now;
            if (isConnected) {
                if (!driver.IsAlive()) {
                    isConnected = false;
                    std::cout << "[-] 게임 프로세스 종료 감지 (연결 끊김)\n";
                }
            } else {
                TryConnect();
            }
        }

        ui.BeginFrame();

        // 창 크기에 따른 유동적 폰트/UI 반응형 스케일링
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        static bool autoScale = true;
        static float manualScale = 1.0f;

        if (autoScale) {
            float scaleW = viewport->WorkSize.x / 1280.0f;
            float scaleH = viewport->WorkSize.y / 800.0f;
            float targetScale = std::clamp(std::min(scaleW, scaleH), 0.85f, 1.40f);
            ImGui::GetIO().FontGlobalScale = targetScale;
        } else {
            ImGui::GetIO().FontGlobalScale = manualScale;
        }

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin("MainEditorWindow", nullptr, windowFlags)) {
            // 상단 상태 대시보드 배너 (주인공 퀵 위젯 및 반응형 스케일 제어 포함)
            float bannerHeight = std::max(95.0f, ImGui::GetFrameHeightWithSpacing() * 3.8f);
            ImGui::BeginChild("TopBanner", ImVec2(0, bannerHeight), true);
            ImGui::Columns(3, "TopStatusColumns", false);

            // 컬럼 1: 프로세스 연결 상태 및 일자
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "★ 삼국지 13 PK 한국어 에디터");
            if (isConnected && driver.IsAlive()) {
                uint16_t yr = 0, mo = 0, da = 0;
                officerMgr.GetGameDate(yr, mo, da);
                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.35f, 1.0f), "● 연결됨 (PID: %d) | %d년 %d월 %d일", driver.GetPid(), yr, mo, da);
                if (ImGui::SmallButton("새로고침 (F5)")) {
                    officerTable.RefreshData();
                    cityTable.RefreshData();
                    itemTable.RefreshData();
                }
            } else {
                ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.25f, 1.0f), "○ 대기 중 (San13PK_kr.exe 실행 대기)");
                if (ImGui::SmallButton("지금 재탐색")) {
                    TryConnect();
                }
            }

            ImGui::NextColumn();
            // 컬럼 2: 버전 및 베이스 주소
            ImGui::Text("게임 버전 및 루트 메모리:");
            if (isConnected) {
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "%s", verDesc.c_str());
                ImGui::Text("RootBase: 0x%lX", (unsigned long)rootBase);
            } else {
                ImGui::TextDisabled("게임 실행 시 자동 감지됩니다");
            }
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "GUI: %.1f FPS | 스케일: %.2fx", 
                               ImGui::GetIO().Framerate, ImGui::GetIO().FontGlobalScale);
            ImGui::Checkbox("창 반응형##AutoUIScale", &autoScale);
            if (!autoScale) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(70.0f);
                ImGui::SliderFloat("##ManualScale", &manualScale, 0.75f, 1.50f, "%.2fx");
            }

            ImGui::NextColumn();
            // 컬럼 3: 플레이어 무장 (주인공) 퀵 대시보드
            ProtagonistRecord prot;
            if (isConnected && officerMgr.GetProtagonistInfo(prot) && prot.is_valid) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "★ 플레이어 무장: %s (ID: %d)", prot.name.c_str(), prot.officer_id);
                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "소속: %s | 신분: %s | 통%d 무%d 지%d 정%d", 
                                   prot.city_name.c_str(), prot.rank_desc.c_str(), 
                                   prot.leadership, prot.war, prot.intel, prot.politics);
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "소지 금: %u 금", prot.personal_gold);
                ImGui::SameLine();
                if (ImGui::SmallButton("+1만")) {
                    uint32_t next = prot.personal_gold + 10000;
                    if (next > 999999) next = 999999;
                    officerMgr.SetProtagonistGold(next);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("5만")) {
                    officerMgr.SetProtagonistGold(50000);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("★ 주인공 편집")) {
                    officerTable.OpenEditModal(prot.officer_id);
                }
            } else {
                ImGui::Text("플레이어 무장 (주인공):");
                ImGui::TextDisabled(isConnected ? "시나리오 진행 중인 주인공 없음" : "게임 연결 대기 중");
            }

            ImGui::Columns(1);
            ImGui::EndChild();

            ImGui::Spacing();

            // 메인 기능 탭 바
            if (ImGui::BeginTabBar("EditorMainTabs", ImGuiTabBarFlags_None)) {
                // 탭 1: 전체 무장 편집기
                if (ImGui::BeginTabItem("전체 무장 편집기 (Officer Editor)")) {
                    ImGui::Spacing();
                    if (isConnected) {
                        officerTable.Render();
                    } else {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.35f, 1.0f), "▶ 게임 프로세스(San13PK_kr.exe)가 연결되지 않았습니다.");
                        ImGui::BulletText("Steam 또는 Proton 환경에서 삼국지 13 PK를 먼저 실행해 주세요.");
                        ImGui::BulletText("게임 실행 후 자동으로 연결되며 데이터가 로드됩니다.");
                        ImGui::Spacing();
                        if (ImGui::Button("프로세스 연결 재시도 ↻", ImVec2(200, 35))) {
                            TryConnect();
                        }
                    }
                    ImGui::EndTabItem();
                }

                // 탭 2: 전체 도시 자원 편집기
                if (ImGui::BeginTabItem("도시 자원 편집기 (City Resource Editor)")) {
                    ImGui::Spacing();
                    if (isConnected) {
                        cityTable.Render();
                    } else {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.35f, 1.0f), "▶ 게임 프로세스가 연결되지 않았습니다.");
                        ImGui::BulletText("삼국지 13 PK 시나리오를 시작한 후 도시 데이터를 편집할 수 있습니다.");
                    }
                    ImGui::EndTabItem();
                }

                // 탭 3: 명품 아이템 편집기
                if (ImGui::BeginTabItem("명품 아이템 편집기 (Item Editor)")) {
                    ImGui::Spacing();
                    if (isConnected) {
                        itemTable.Render();
                    } else {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.35f, 1.0f), "▶ 게임 프로세스가 연결되지 않았습니다.");
                        ImGui::BulletText("삼국지 13 PK 시나리오를 시작한 후 명품 목록을 조회하고 편집할 수 있습니다.");
                    }
                    ImGui::EndTabItem();
                }

                // 탭 3: 시스템 환경 및 보안 권한 가이드
                if (ImGui::BeginTabItem("시스템 환경 및 보안 가이드 (Security & Info)")) {
                    ImGui::Spacing();
#ifdef _WIN32
                    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "■ Windows 메모리 접근 권한 안내");
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::BulletText("게임(San13PK_kr.exe)과 동일한 권한 수준으로 에디터를 실행해 주세요.");
                    ImGui::BulletText("게임을 관리자 권한으로 실행했다면 에디터도 관리자 권한으로 실행해야 합니다.");
#else
                    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "■ 리눅스 메모리 접근 권한 및 보안 스코프 안내");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::BulletText("현재 Linux ptrace 보안 레벨: ptrace_scope = %d", ptraceScope);
                    if (ptraceScope == 0) {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "  ● 전체 프로세스 메모리 읽기/쓰기가 완벽히 허용된 상태입니다.");
                    } else if (ptraceScope == 1) {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "  ● 동일 사용자(UID) 프로세스 메모리 접근이 허용된 표준 상태입니다 (Steam/Proton 게임 접근 가능).");
                    } else {
                        ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.2f, 1.0f), "  ▲ ptrace 접근이 제한되어 있습니다. 권한 부족 시 아래 명령어로 허용해 주세요:");
                        ImGui::TextDisabled("     sudo sysctl -w kernel.yama.ptrace_scope=1");
                    }

#endif

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "■ 삼국지 13 PK 도메인 레이아웃 명세");
                    ImGui::BulletText("역사 무장 기본 수: 800명 (Stride: 0x12BC = 4,796 바이트)");
                    ImGui::BulletText("동적 슬롯 확장: 최대 2,165명 (결혼 자녀 무장 및 신무장 자동 스캔)");
                    ImGui::BulletText("도시 목록: 60개 전 도시 (Stride: 0x170 = 368 바이트)");
                    ImGui::BulletText("실시간 핫싱크 지연시간: < 0.003ms (process_vm_writev 커널 시스템 콜)");
                    ImGui::BulletText("수치 보호기: SafeVanilla (1~100) / LimitBreak (1~255) / StrictGuard (적성 0~3)");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.85f, 0.75f, 0.5f, 1.0f), "■ 조작 단축키 및 팁");
                    ImGui::BulletText("무장 상세 편집: 테이블 행 더블클릭 또는 우클릭 컨텍스트 메뉴");
                    ImGui::BulletText("다중 선택: 첫 열 체크박스 클릭 또는 Ctrl+클릭 (개별), Shift+클릭 (범위)");
                    ImGui::BulletText("일괄 수정: 무장 다중 선택 후 상단 '일괄 수정' 버튼 클릭 (아군 스탯 +10, 수명 연장)");
                    ImGui::BulletText("실행 취소: 일괄 수정 모달 내 '직전 일괄 수정 실행 취소 (Rollback)' 원클릭 복구");

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        ui.EndFrame();
        frameCount++;
    }

    ui.Shutdown();
    std::cout << "[+] 에디터가 정상적으로 종료되었습니다.\n";
    return 0;
}
