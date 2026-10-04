#include "ui/UIManager.hpp"
#include "ui/FontHelper.hpp"

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>

namespace San13::UI {

static void GlfwErrorCallback(int error, const char* description) {
    std::cerr << "[GLFW Error " << error << "]: " << (description ? description : "알 수 없는 오류") << "\n";
}

UIManager::UIManager()
    : m_config(WindowConfig()) {
}

UIManager::UIManager(const WindowConfig& config)
    : m_config(config) {
}

UIManager::~UIManager() {
    Shutdown();
}

bool UIManager::Initialize() {
    if (m_initialized) return true;

    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "[UIManager] Error: GLFW 초기화 실패\n";
        return false;
    }

    // OpenGL 3.3 Core Profile 설정
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(m_config.width, m_config.height, m_config.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "[UIManager] Error: GLFW 창 생성 실패\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(m_config.vsync ? 1 : 0);

    // Dear ImGui 컨텍스트 생성
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 고유 삼국지 다크 테마 적용
    ApplyTheme();

    // 완성형 한글 폰트 로드
    FontHelper::LoadKoreanFont(io, m_config.fontSize, m_config.fontPath);

    // 백엔드 초기화
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true)) {
        std::cerr << "[UIManager] Error: ImGui GLFW 백엔드 초기화 실패\n";
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "[UIManager] Error: ImGui OpenGL3 백엔드 초기화 실패\n";
        return false;
    }

    m_initialized = true;
    std::cout << "[UIManager] GUI 서브시스템 초기화 완료 (" << m_config.width << "x" << m_config.height << ")\n";
    return true;
}

bool UIManager::ShouldClose() const {
    if (!m_window) return true;
    return glfwWindowShouldClose(m_window);
}

void UIManager::BeginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::EndFrame() {
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    // 삼국지 풍의 짙은 차콜 배경
    glClearColor(0.09f, 0.10f, 0.12f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}

void UIManager::Shutdown() {
    if (!m_initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();

    m_initialized = false;
    std::cout << "[UIManager] GUI 서브시스템 안전하게 종료됨\n";
}

void UIManager::ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 삼국지 풍의 앤틱 다크 골드 & 브론즈 / 오크 차콜 테마 팔레트
    colors[ImGuiCol_Text]                  = ImVec4(0.93f, 0.92f, 0.89f, 1.00f); // 따뜻한 미색
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.52f, 0.50f, 0.46f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.11f, 0.12f, 0.13f, 0.98f); // 딥 차콜 오크
    colors[ImGuiCol_ChildBg]               = ImVec4(0.09f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.13f, 0.13f, 0.15f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.32f, 0.28f, 0.22f, 0.75f); // 은은한 앤틱 브론즈 테두리
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 0.24f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.32f, 0.29f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.20f, 0.18f, 0.15f, 1.00f); // 활성 창 황실 브라운
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.08f, 0.08f, 0.09f, 0.80f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.09f, 0.09f, 0.10f, 0.65f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.28f, 0.26f, 0.24f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.48f, 0.42f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.68f, 0.56f, 0.36f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.95f, 0.80f, 0.35f, 1.00f); // 찬란한 황금색
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.78f, 0.64f, 0.32f, 1.00f); // 앤틱 골드 슬라이더
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.96f, 0.82f, 0.42f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.21f, 1.00f); // 묵직한 버튼
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.60f, 0.46f, 0.20f, 0.85f); // 앤틱 골드 호버
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.78f, 0.62f, 0.26f, 1.00f); // 골드 클릭
    colors[ImGuiCol_Header]                = ImVec4(0.22f, 0.21f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.55f, 0.44f, 0.22f, 0.75f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.72f, 0.58f, 0.28f, 0.90f);
    colors[ImGuiCol_Separator]             = ImVec4(0.28f, 0.26f, 0.23f, 0.85f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.75f, 0.60f, 0.30f, 0.85f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.90f, 0.75f, 0.38f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.28f, 0.25f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.78f, 0.64f, 0.32f, 0.70f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.92f, 0.78f, 0.42f, 0.95f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.14f, 0.14f, 0.16f, 0.95f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.62f, 0.48f, 0.22f, 0.85f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.26f, 0.24f, 0.21f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.11f, 0.11f, 0.12f, 0.90f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.17f, 0.16f, 1.00f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(0.85f, 0.70f, 0.35f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.80f, 0.65f, 0.32f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.98f, 0.85f, 0.48f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.32f, 0.28f, 0.22f, 0.90f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.22f, 0.20f, 0.18f, 0.65f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 0.95f, 0.85f, 0.025f); // 미세한 웜브라운 교차행

    // 정제된 라운딩 및 패딩
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 5.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 7.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 5.0f;

    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(8.0f, 5.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 13.0f;
}

} // namespace San13::UI
