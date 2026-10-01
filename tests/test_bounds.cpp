#include "game/Bounds.hpp"
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

using namespace San13::Game;

// Test 1: EDGE-01 & EDGE-02 (음수 및 0 입력 시 최소치 1 보정)
bool Test_NegativeAndZeroStats() {
    std::cout << "[Test 1] EDGE-01 & EDGE-02: 무장 스탯 음수 및 0 입력 언더플로 방어\n";
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(-1) == 1, "-1 입력 시 1로 보정");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(-100) == 1, "-100 입력 시 1로 보정");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(0) == 1, "0 입력 시 1로 보정");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(-1, ClampingPolicy::LimitBreak) == 1, "한계돌파 모드에서도 음수는 1로 보정");
    return true;
}

// Test 2: EDGE-03 (바닐라 상한 100 클램핑 vs 한계돌파 허용)
bool Test_VanillaVsLimitBreak() {
    std::cout << "\n[Test 2] EDGE-03: 바닐라 모드 100 상한 vs 한계 돌파 101~255 허용\n";
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(100) == 100, "100 입력 시 100 유지");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(105, ClampingPolicy::SafeVanilla) == 100, "Safe 모드: 105 입력 시 100으로 절삭");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(105, ClampingPolicy::LimitBreak) == 105, "LimitBreak 모드: 105 입력 허용");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(200, ClampingPolicy::LimitBreak) == 200, "LimitBreak 모드: 200 입력 허용");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(255, ClampingPolicy::LimitBreak) == 255, "LimitBreak 모드: 255 입력 허용");
    return true;
}

// Test 3: EDGE-04 (8비트 오버플로 모듈로 방어 - Saturating Cast)
bool Test_HardwareOverflowSaturating() {
    std::cout << "\n[Test 3] EDGE-04: 256 이상 입력 시 비트 오버플로 0 래핑 방어 (Saturating Cast)\n";
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(256, ClampingPolicy::LimitBreak) == 255, "256 입력 시 0이 아닌 255로 포화 고정");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(300, ClampingPolicy::LimitBreak) == 255, "300 입력 시 255로 포화 고정");
    TEST_ASSERT(BoundsValidator::SanitizeOfficerStat(99999, ClampingPolicy::LimitBreak) == 255, "99999 입력 시 255로 포화 고정");
    return true;
}

// Test 4: EDGE-05 (병종 적성 4 이상 크래시 방지 Strict Guard)
bool Test_AptitudeCrashGuard() {
    std::cout << "\n[Test 4] EDGE-05: 병종 적성 스프라이트 인덱스 초과 크래시 방어 (Strict Guard)\n";
    TEST_ASSERT(BoundsValidator::SanitizeAptitude(0) == 0, "적성 C(0) 정상 통과");
    TEST_ASSERT(BoundsValidator::SanitizeAptitude(3) == 3, "적성 S(3) 정상 통과");
    TEST_ASSERT(BoundsValidator::SanitizeAptitude(4) == 3, "적성 4 입력 시 3(S)으로 강제 절삭");
    TEST_ASSERT(BoundsValidator::SanitizeAptitude(10) == 3, "적성 10 입력 시 3(S)으로 강제 절삭");
    TEST_ASSERT(BoundsValidator::SanitizeAptitude(255) == 3, "적성 255 입력 시 3(S)으로 강제 절삭");
    TEST_ASSERT(BoundsValidator::SanitizeAptitude(-1) == 0, "적성 -1 입력 시 0(C)으로 강제 보정");
    return true;
}

// Test 5: EDGE-06 (생몰년도 역전 논리 무결성 검증)
bool Test_LifeSpanIntegrity() {
    std::cout << "\n[Test 5] EDGE-06: 생몰년도 역전(사망 < 출생) 방어 검증\n";
    // 정상: 조조 (155 출생, 184 등장, 220 사망)
    TEST_ASSERT(BoundsValidator::ValidateLifeSpan(155, 184, 220), "조조 정상 생몰년도(155<=184<=220) 통과");
    // 비정상: 사망년도가 출생년도보다 빠름
    TEST_ASSERT(!BoundsValidator::ValidateLifeSpan(200, 210, 180), "사망(180) < 출생(200) 역전 시 거부");
    // 비정상: 등장년도가 출생년도보다 빠름
    TEST_ASSERT(!BoundsValidator::ValidateLifeSpan(200, 190, 250), "등장(190) < 출생(200) 역전 시 거부");
    // 비정상: 0년 또는 범위 밖 연도
    TEST_ASSERT(!BoundsValidator::ValidateLifeSpan(0, 10, 50), "100년 미만 비정상 연도 거부");
    return true;
}

// Test 6: EDGE-07 & EDGE-08 (도시 자원 6자리 초과 및 내구도 16비트 오버플로 방어)
bool Test_CitySuppliesAndDurability() {
    std::cout << "\n[Test 6] EDGE-07 & EDGE-08: 도시 자원/내구도 오버플로 방어\n";
    // 자금/군량
    TEST_ASSERT(BoundsValidator::SanitizeCitySupplies(999999) == 999999, "999,999 정상 통과");
    TEST_ASSERT(BoundsValidator::SanitizeCitySupplies(1000000, ClampingPolicy::SafeVanilla) == 999999, "Safe 모드: 100만 이상은 999,999로 절삭");
    TEST_ASSERT(BoundsValidator::SanitizeCitySupplies(1500000, ClampingPolicy::LimitBreak) == 1500000, "LimitBreak 모드: 150만 허용");
    TEST_ASSERT(BoundsValidator::SanitizeCitySupplies(-500) == 0, "음수 자금은 0으로 보정");
    // 내구도
    TEST_ASSERT(BoundsValidator::SanitizeCityDurability(5000) == 5000, "내구도 5,000 정상 통과");
    TEST_ASSERT(BoundsValidator::SanitizeCityDurability(65536) == 5000, "65536 입력 시 16비트 오버플로 0 방지 (5000 절삭)");
    return true;
}

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 4.2] Value Bounds & QA Edge Case Test Suite\n";
    std::cout << "============================================================\n\n";

    int passed = 0;
    int total = 6;

    if (Test_NegativeAndZeroStats()) passed++;
    if (Test_VanillaVsLimitBreak()) passed++;
    if (Test_HardwareOverflowSaturating()) passed++;
    if (Test_AptitudeCrashGuard()) passed++;
    if (Test_LifeSpanIntegrity()) passed++;
    if (Test_CitySuppliesAndDurability()) passed++;

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: " << passed << " / " << total << " 개 테스트 세트 통과\n";
    std::cout << "============================================================\n";

    if (passed == total) {
        std::cout << ">>> \033[32m[PHASE 4.2 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";
        return 0;
    } else {
        std::cerr << ">>> \033[31m[PHASE 4.2 실패]\033[0m <<<\n\n";
        return 1;
    }
}
