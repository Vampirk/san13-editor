#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace San13::Game {

#pragma pack(push, 1)

/**
 * @brief 삼국지 13 PK 도시(City) 오브젝트 메모리 레이아웃 (368 바이트 = 0x170 Stride)
 * 실시간 Proton 라이브 메모리 실측 및 2개 이상 도시 교차 검증을 통해 오프셋 검증 완료.
 */
struct San13CityRaw {
    uint8_t  head_pad[0x58];          // +0x00 ~ +0x57: VTable(0x10), 세력/군주/태수 소속 정보
    uint32_t durability;              // +0x58: 도시 내구도 [확정]
    uint32_t gold;                    // +0x5C: 자금 [확정]
    uint32_t provisions;              // +0x60: 군량 [확정]
    uint32_t troops;                  // +0x64: 주둔 병력 [확정]
    uint8_t  pad_0x68[0x6A];          // +0x68 ~ +0xD1: 부상병/귀환병 후보 영역 (106B)
    uint16_t public_order;            // +0x0D2: 민심 (양평 100, 금성 350 실측) [확정]
    uint8_t  pad_0xD4[2];             // +0x0D4 ~ +0x0D5: 패딩 (2B)
    uint16_t commerce;                // +0x0D6: 상업 기본 개발치 (양평 1000 실측) [유력]
    uint8_t  pad_0xD8[8];             // +0x0D8 ~ +0x0DF: 패딩 (8B)
    uint16_t agriculture;             // +0x0E0: 농업 기본 개발치 (양평 1050, 금성 2000 실측) [확정]
    uint8_t  pad_0xE2[8];             // +0x0E2 ~ +0x0E9: 패딩 (8B)
    uint16_t culture;                 // +0x0EA: 문화 기본 개발치 (양평 1000, 금성 2400 실측) [확정]
    uint8_t  pad_0xEC[0x10];          // +0x0EC ~ +0x0FB: 패딩 (16B)
    uint16_t spear_proficiency;       // +0x0FC: 창병 숙련도 (양평 200, 금성 700 실측) [확정]
    uint16_t horse_proficiency;       // +0x0FE: 기병 숙련도 (양평 300, 금성 700 실측) [확정]
    uint16_t bow_proficiency;         // +0x100: 궁병 숙련도 (양평 200, 금성 700 실측) [확정]
    uint8_t  pad_0x102[0x42];         // +0x102 ~ +0x143: 내부 vtable 및 통계 전방 패딩 (66B)
    uint32_t officer_count;           // +0x144: 소속 무장 수 (금성 12명 실측) [유력]
    uint8_t  pad_0x148[4];            // +0x148 ~ +0x14B: 소속 무장 포인터 영역 (4B)
    uint32_t prisoner_count;          // +0x14C: 수감 포로 수 (금성 0명 실측) [유력]
    uint8_t  pad_0x150[4];            // +0x150 ~ +0x153: 수감 포로 포인터 영역 (4B)
    uint32_t free_officer_count;      // +0x154: 재야 무장 수 (금성 1명 실측) [유력]
    uint8_t  tail_pad[0x18];          // +0x158 ~ +0x16F: 청크 말미 패딩 (24B)
};

#pragma pack(pop)

static_assert(sizeof(San13CityRaw) == 0x170, "San13CityRaw size must be exactly 0x170 (368 bytes)");
static_assert(offsetof(San13CityRaw, durability) == 0x58, "durability offset mismatch");
static_assert(offsetof(San13CityRaw, gold) == 0x5C, "gold offset mismatch");
static_assert(offsetof(San13CityRaw, provisions) == 0x60, "provisions offset mismatch");
static_assert(offsetof(San13CityRaw, troops) == 0x64, "troops offset mismatch");
static_assert(offsetof(San13CityRaw, public_order) == 0xD2, "public_order offset mismatch");
static_assert(offsetof(San13CityRaw, commerce) == 0xD6, "commerce offset mismatch");
static_assert(offsetof(San13CityRaw, agriculture) == 0xE0, "agriculture offset mismatch");
static_assert(offsetof(San13CityRaw, culture) == 0xEA, "culture offset mismatch");
static_assert(offsetof(San13CityRaw, spear_proficiency) == 0xFC, "spear_proficiency offset mismatch");
static_assert(offsetof(San13CityRaw, horse_proficiency) == 0xFE, "horse_proficiency offset mismatch");
static_assert(offsetof(San13CityRaw, bow_proficiency) == 0x100, "bow_proficiency offset mismatch");
static_assert(offsetof(San13CityRaw, officer_count) == 0x144, "officer_count offset mismatch");
static_assert(offsetof(San13CityRaw, prisoner_count) == 0x14C, "prisoner_count offset mismatch");
static_assert(offsetof(San13CityRaw, free_officer_count) == 0x154, "free_officer_count offset mismatch");

struct CityMemoryLayout {
    static constexpr uintptr_t CHUNK_OFFSET = 0x007178;  // RootBase로부터의 오프셋
    static constexpr size_t    OBJECT_STRIDE = 0x170;    // 단일 도시 크기 (368 바이트)
    static constexpr uint8_t   MAX_CITY_COUNT = 60;      // 삼국지 13 PK 총 도시 수
};

/**
 * @brief UI 및 비즈니스 로직용 도시 레코드
 */
struct CityRecord {
    uint8_t id = 0;
    std::string name;
    // 기존 4대 자원
    uint32_t gold = 0;
    uint32_t provisions = 0;
    uint32_t troops = 0;
    uint32_t durability = 0;

    // 내정 4대치 (읽기 전용)
    uint16_t public_order = 0;       // 민심
    uint16_t commerce = 0;           // 상업 기본치
    uint16_t agriculture = 0;        // 농업 기본치
    uint16_t culture = 0;            // 문화 기본치

    // 병과 3대 숙련도 (읽기 전용)
    uint16_t spear_proficiency = 0;  // 창병 숙련도
    uint16_t horse_proficiency = 0;  // 기병 숙련도
    uint16_t bow_proficiency = 0;    // 궁병 숙련도

    // 소속 인물 통계 (읽기 전용)
    uint32_t officer_count = 0;      // 소속 무장 수
    uint32_t prisoner_count = 0;     // 수감 포로 수
    uint32_t free_officer_count = 0; // 재야 무장 수

    uintptr_t memory_address = 0;
};

/**
 * @brief 삼국지 13 PK 60개 도시 한글 명칭 헬퍼
 */
const char* GetCityName(uint8_t id);

} // namespace San13::Game
