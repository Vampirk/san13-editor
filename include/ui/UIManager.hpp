#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

namespace San13::UI {

struct WindowConfig {
    std::string title = "삼국지 13 PK 한국어 트레이너 & 에디터";
    int width = 1280;
    int height = 800;
    bool vsync = true;
    float fontSize = 18.0f;
    std::string fontPath = "";
};

/**
 * @brief GLFW + OpenGL3 + Dear ImGui 라이프사이클 및 창 관리자
 */
class UIManager {
public:
    UIManager();
    explicit UIManager(const WindowConfig& config);
    ~UIManager();

    /**
     * @brief GLFW 및 OpenGL3, ImGui 컨텍스트 초기화
     */
    bool Initialize();

    /**
     * @brief 창 닫힘 요청 여부
     */
    bool ShouldClose() const;

    /**
     * @brief 프레임 렌더링 시작 (이벤트 폴링 및 ImGui NewFrame)
     */
    void BeginFrame();

    /**
     * @brief 프레임 렌더링 완료 및 버퍼 교환
     */
    void EndFrame();

    /**
     * @brief 리소스 해제 및 정상 종료
     */
    void Shutdown();

    /**
     * @brief 삼국지 테마 (다크 브론즈 & 골드 악센트) 스타일 적용
     */
    void ApplyTheme();

    GLFWwindow* GetWindow() const { return m_window; }
    bool IsInitialized() const { return m_initialized; }

private:
    WindowConfig m_config;
    GLFWwindow*  m_window = nullptr;
    bool         m_initialized = false;
};

} // namespace San13::UI
