#pragma once

#include <string>
#include "imgui.h"

namespace San13::UI {

/**
 * @brief 한글 폰트 로드 및 글리프 아틀라스 빌더 헬퍼
 * 깨짐 없는 완벽한 한글 표시(완성형 11,172자, 자모음, 기호)를 보장합니다.
 */
class FontHelper {
public:
    /**
     * @brief 기본 및 시스템 한글 폰트를 탐색하여 ImGui 폰트 아틀라스에 등록합니다.
     * @param io ImGuiIO 참조
     * @param fontSize 폰트 크기 (기본값: 18.0f)
     * @param customFontPath 사용자가 지정한 커스텀 폰트 경로 (비어있으면 기본 번들/시스템 폰트 자동 탐색)
     * @return true 폰트 로드 성공, false 실패(기본 폰트로 폴백)
     */
    static bool LoadKoreanFont(ImGuiIO& io, float fontSize = 18.0f, const std::string& customFontPath = "");

    /**
     * @brief 현재 시스템에서 사용 가능한 유효한 한글 폰트 경로를 반환합니다.
     */
    static std::string FindKoreanFontPath(const std::string& preferredPath = "");
};

} // namespace San13::UI
