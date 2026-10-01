#include "ui/FontHelper.hpp"
#include <iostream>
#include <fstream>
#include <vector>

namespace San13::UI {

static bool FileExists(const std::string& path) {
    if (path.empty()) return false;
    std::ifstream f(path.c_str());
    return f.good();
}

std::string FontHelper::FindKoreanFontPath(const std::string& preferredPath) {
    if (!preferredPath.empty() && FileExists(preferredPath)) {
        return preferredPath;
    }

    // 우선순위 1: 배포 번들 폰트 (assets/fonts)
    const std::vector<std::string> candidatePaths = {
        "assets/fonts/NanumGothic.ttf",
        "../assets/fonts/NanumGothic.ttf",
        "../../assets/fonts/NanumGothic.ttf",
        // Linux 시스템 폰트 경로
        "/usr/share/fonts/naver-nanum/NanumGothic.ttf",
        "/usr/share/fonts/google-noto-sans-cjk-vf-fonts/NotoSansCJK-VF.ttc",
        "/usr/share/fonts/google-noto-cjk/NotoSansCJK-KR.ttc",
        "/usr/share/fonts/nanum/NanumGothic.ttf",
        // Windows 시스템 폰트 경로 (크로스 플랫폼 호환)
        "C:\\Windows\\Fonts\\malgun.ttf",
        "C:\\Windows\\Fonts\\NanumGothic.ttf"
    };

    for (const auto& path : candidatePaths) {
        if (FileExists(path)) {
            return path;
        }
    }

    return "";
}

bool FontHelper::LoadKoreanFont(ImGuiIO& io, float fontSize, const std::string& customFontPath) {
    std::string fontPath = FindKoreanFontPath(customFontPath);
    if (fontPath.empty()) {
        std::cerr << "[FontHelper] Warning: 유효한 한글 폰트를 찾지 못했습니다. ImGui 기본 폰트를 사용합니다.\n";
        return false;
    }

    // 한글 글리프 아틀라스 범위 구성
    static ImVector<ImWchar> s_glyphRanges;
    s_glyphRanges.clear();

    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesKorean());

    // 삼국지 UI 및 트레이너에 유용한 특수 기호 추가
    builder.AddText("→←↑↓▲▼★☆※●○■□◆◇▶◀+-%×÷·…");
    builder.BuildRanges(&s_glyphRanges);

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 1;
    fontConfig.PixelSnapH = true;

    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &fontConfig, s_glyphRanges.Data);
    if (!font) {
        std::cerr << "[FontHelper] Error: 폰트 파일 로드 실패: " << fontPath << "\n";
        return false;
    }

    std::cout << "[FontHelper] 한글 폰트 로드 성공: " << fontPath << " (크기: " << fontSize << "px)\n";
    return true;
}

} // namespace San13::UI
