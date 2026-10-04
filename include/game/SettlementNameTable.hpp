#pragma once

#include <cstdint>
#include <string>

namespace San13::Game {

/**
 * @brief 삼국지 13 PK 부락(集落, Settlement) 한글 지명 매핑 테이블
 * 300개 부락(ID 0 ~ 299)에 대한 실제 인게임 지명 매핑 및 스마트 포맷팅 지원
 */
class SettlementNameTable {
public:
    /**
     * @brief 부락 ID(0 ~ 299)에 해당하는 고유 한글 지명 반환 (미등록 시 nullptr)
     */
    static const char* GetSettlementName(uint16_t id);

    /**
     * @brief UI 표시용 최적화 포맷팅 명칭 반환
     * 등록된 지명이 있을 경우: "호로관", "밀", "옹구" 등
     * 미등록일 경우: cityName이 있으면 "[낙양] 부락 #017", 없으면 "부락 #017"
     */
    static std::string GetFormattedName(uint16_t id, const std::string& cityName = "");

    /**
     * @brief 부락 효과 ID(0 ~ 43)에 해당하는 고유 명칭 반환 (예: "호표기", "인구+20000", 미등록 시 nullptr)
     */
    static const char* GetEffectName(uint8_t effectId);

    /**
     * @brief 부락에 부여된 최대 4개 효과 ID 목록을 문자열로 포맷팅 (예: "호표기", "인구+20000, 인구+40000")
     */
    static std::string GetEffectDescription(const uint8_t buff_effects[4]);
};

} // namespace San13::Game
