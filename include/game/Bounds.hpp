#pragma once

#include <cstdint>
#include <algorithm>

namespace San13::Game {

/**
 * @brief 수치 편집 정책 (안전 모드 vs 한계 돌파 모드)
 */
enum class ClampingPolicy {
    SafeVanilla,   // 게임 원본 UI 규격 및 밸런스 기준 (예: 능력치 1 ~ 100)
    LimitBreak,    // 물리 한계치 허용 + 오버플로 방지 포화 연산 (예: 능력치 1 ~ 255)
    Strict         // 시스템 무결성을 위해 한계 돌파를 절대 불허 (예: 병종 적성 0~3)
};

/**
 * @brief 수치별 물리/논리 경계값 정의
 */
struct StatBounds {
    // 1. 무장 4대 스탯 (통솔, 무력, 지력, 정치) - 메모리: uint8_t (1바이트)
    struct OfficerStats {
        static constexpr uint8_t  MIN_VAL = 1;
        static constexpr uint8_t  VANILLA_MAX = 100;
        static constexpr uint8_t  HARDWARE_MAX = 255;  // 256 이상 입력 시 모듈로(% 256)로 0이 되는 현상 방지
    };

    // 2. 병종 적성 (창병, 기병, 궁병) - 메모리: uint8_t (0:C, 1:B, 2:A, 3:S)
    // 4 이상 시 게임 UI 스프라이트 인덱스 범위 초과로 세그폴트 크래시 유발하므로 0~3 엄격 고정
    struct OfficerAptitude {
        static constexpr uint8_t  MIN_VAL = 0;
        static constexpr uint8_t  MAX_VAL = 3;
    };

    // 3. 무장 능력치 경험치 (0 ~ 99)
    struct OfficerExp {
        static constexpr uint8_t  MIN_VAL = 0;
        static constexpr uint8_t  MAX_VAL = 99;
    };

    // 4. 생몰년도 (출생, 등장, 사망) - 메모리: uint16_t
    struct OfficerYear {
        static constexpr uint16_t MIN_YEAR = 100;
        static constexpr uint16_t MAX_YEAR = 400;
    };

    // 5. 도시 자금 / 군량 - 메모리: uint32_t (4바이트)
    struct CitySupplies {
        static constexpr uint32_t MIN_VAL = 0;
        static constexpr uint32_t VANILLA_MAX = 999999;
        static constexpr uint32_t LIMIT_BREAK_MAX = 9999999;
    };

    // 6. 도시 병력 - 메모리: uint32_t
    struct CityTroops {
        static constexpr uint32_t MIN_VAL = 0;
        static constexpr uint32_t VANILLA_MAX = 200000;
        static constexpr uint32_t LIMIT_BREAK_MAX = 999999;
    };

    // 7. 도시 내구도 - 메모리: uint16_t (2바이트)
    struct CityDurability {
        static constexpr uint16_t MIN_VAL = 0;
        static constexpr uint16_t VANILLA_MAX = 5000;
        static constexpr uint16_t LIMIT_BREAK_MAX = 10000;
    };

    // 8. 명품 능력 가산치 - 메모리: int8_t (1바이트 부호 있는 정수)
    struct ItemBonus {
        static constexpr int8_t   MIN_VAL = -50;
        static constexpr int8_t   MAX_VAL = 50;
    };
};

/**
 * @brief 수치 검증 및 오버플로 방지 변환기 (BoundsValidator)
 */
class BoundsValidator {
public:
    // 무장 스탯 안전 변환 (EDGE-01 ~ EDGE-04)
    static inline uint8_t SanitizeOfficerStat(int64_t input, ClampingPolicy policy = ClampingPolicy::SafeVanilla) {
        if (input < StatBounds::OfficerStats::MIN_VAL) {
            return StatBounds::OfficerStats::MIN_VAL;
        }

        uint8_t max_allowed = (policy == ClampingPolicy::LimitBreak)
                              ? StatBounds::OfficerStats::HARDWARE_MAX
                              : StatBounds::OfficerStats::VANILLA_MAX;

        if (input > max_allowed) {
            return max_allowed; // Saturating Cast (포화 연산: 256 입력 시 0이 아닌 255 고정)
        }

        return static_cast<uint8_t>(input);
    }

    // 병종 적성 안전 변환 (EDGE-05: 4 이상 입력 시 크래시 원천 차단)
    static inline uint8_t SanitizeAptitude(int64_t input) {
        if (input < StatBounds::OfficerAptitude::MIN_VAL) return StatBounds::OfficerAptitude::MIN_VAL;
        if (input > StatBounds::OfficerAptitude::MAX_VAL) return StatBounds::OfficerAptitude::MAX_VAL;
        return static_cast<uint8_t>(input & 0x03);
    }

    // 능력치 경험치 안전 변환
    static inline uint8_t SanitizeExp(int64_t input) {
        if (input < StatBounds::OfficerExp::MIN_VAL) return StatBounds::OfficerExp::MIN_VAL;
        if (input > StatBounds::OfficerExp::MAX_VAL) return StatBounds::OfficerExp::MAX_VAL;
        return static_cast<uint8_t>(input);
    }

    // 생몰년도 논리적 무결성 검증 (EDGE-06: 출생 <= 등장 <= 사망)
    static inline bool ValidateLifeSpan(uint16_t birth, uint16_t appear, uint16_t death) {
        if (birth < StatBounds::OfficerYear::MIN_YEAR || death > StatBounds::OfficerYear::MAX_YEAR) {
            return false;
        }
        return (birth <= appear && appear <= death);
    }

    // 도시 자금/군량 안전 변환 (EDGE-07)
    static inline uint32_t SanitizeCitySupplies(int64_t input, ClampingPolicy policy = ClampingPolicy::SafeVanilla) {
        if (input < 0) return 0;
        uint32_t max_allowed = (policy == ClampingPolicy::LimitBreak)
                               ? StatBounds::CitySupplies::LIMIT_BREAK_MAX
                               : StatBounds::CitySupplies::VANILLA_MAX;
        if (input > max_allowed) return max_allowed;
        return static_cast<uint32_t>(input);
    }

    // 도시 병력 안전 변환
    static inline uint32_t SanitizeCityTroops(int64_t input, ClampingPolicy policy = ClampingPolicy::SafeVanilla) {
        if (input < 0) return 0;
        uint32_t max_allowed = (policy == ClampingPolicy::LimitBreak)
                               ? StatBounds::CityTroops::LIMIT_BREAK_MAX
                               : StatBounds::CityTroops::VANILLA_MAX;
        if (input > max_allowed) return max_allowed;
        return static_cast<uint32_t>(input);
    }

    // 도시 내구도 안전 변환 (EDGE-08)
    static inline uint16_t SanitizeCityDurability(int64_t input, ClampingPolicy policy = ClampingPolicy::SafeVanilla) {
        if (input < 0) return 0;
        uint16_t max_allowed = (policy == ClampingPolicy::LimitBreak)
                               ? StatBounds::CityDurability::LIMIT_BREAK_MAX
                               : StatBounds::CityDurability::VANILLA_MAX;
        if (input > max_allowed) return max_allowed;
        return static_cast<uint16_t>(input);
    }

    // 명품 능력치 가산치 안전 변환
    static inline int8_t SanitizeItemBonus(int64_t input) {
        if (input < StatBounds::ItemBonus::MIN_VAL) return StatBounds::ItemBonus::MIN_VAL;
        if (input > StatBounds::ItemBonus::MAX_VAL) return StatBounds::ItemBonus::MAX_VAL;
        return static_cast<int8_t>(input);
    }
};

} // namespace San13::Game
