#pragma once

#include <cstdint>
#include <string>
#include <cstddef>

namespace San13::Game {

#pragma pack(push, 1)
// 삼국지 13 PK 명품(아이템) 바이너리 원본 구조체 (Stride: 36바이트, 0x24)
// 주소: RootBase + 0x9FE8CC + (item_id * 0x24)
struct San13ItemRaw {
    uint32_t vtable;             // +0x00: VTable 포인터 (0x72919da8)
    uint16_t item_id;            // +0x04: 고유 명품 ID (0 ~ 399)
    uint8_t  category;           // +0x06: 대분류 (0:명마, 1:검, 2:도, 3:대도, 4:창/모, 5:극, 6:부, 7:편, 8:추, 9:암기, 10:궁, 11:병서, 12:경서, 13:사서, 14:자서, 15:문집, 16:의서, 17:정서, 18:지도, 19:보물, 20:주류, 21:약재 등)
    uint8_t  sub_category;       // +0x07: 소분류 (0xFF=없음)
    uint8_t  horse_speed;        // +0x08: 명마 이동속도/기동력 보정
    uint8_t  pad_09;             // +0x09: 패딩
    uint8_t  stat_type;          // +0x0A: 능력치 종류 (1:무력, 2:통솔, 3:지력, 4:정치, 0:없음)
    uint8_t  pad_0B;             // +0x0B: 패딩
    uint8_t  stat_boost;         // +0x0C: 능력치 상승폭 (+1 ~ +10)
    uint8_t  pad_0D;             // +0x0D: 패딩
    uint8_t  skill_id;           // +0x0E: 부가 특기 ID (0:농업 ~ 19:귀모, 0xFF:없음)
    uint8_t  skill_level;        // +0x0F: 부가 특기 레벨 (1 ~ 9)
    uint16_t value;              // +0x10: 명품 가치 (1 ~ 100)
    uint16_t price;              // +0x12: 기본 가격 (금)
    uint8_t  pad_14[4];          // +0x14 ~ +0x17: 패딩
    uint16_t original_id;        // +0x18: 원본 인덱스
    uint8_t  pad_1A[2];          // +0x1A ~ +0x1B: 패딩
    uint16_t owner_officer_id;   // +0x1C: 소유 무장 고유 ID (0xFFFF = 미소유/시장/상인)
    uint8_t  pad_1E[2];          // +0x1E ~ +0x1F: 패딩
    uint32_t flags;              // +0x20: 상태 플래그
};
#pragma pack(pop)

static_assert(sizeof(San13ItemRaw) == 36, "San13ItemRaw struct must be exactly 36 bytes (0x24)");
static_assert(offsetof(San13ItemRaw, item_id) == 0x04, "item_id offset mismatch");
static_assert(offsetof(San13ItemRaw, category) == 0x06, "category offset mismatch");
static_assert(offsetof(San13ItemRaw, horse_speed) == 0x08, "horse_speed offset mismatch");
static_assert(offsetof(San13ItemRaw, stat_type) == 0x0A, "stat_type offset mismatch");
static_assert(offsetof(San13ItemRaw, stat_boost) == 0x0C, "stat_boost offset mismatch");
static_assert(offsetof(San13ItemRaw, skill_id) == 0x0E, "skill_id offset mismatch");
static_assert(offsetof(San13ItemRaw, skill_level) == 0x0F, "skill_level offset mismatch");
static_assert(offsetof(San13ItemRaw, value) == 0x10, "value offset mismatch");
static_assert(offsetof(San13ItemRaw, price) == 0x12, "price offset mismatch");
static_assert(offsetof(San13ItemRaw, owner_officer_id) == 0x1C, "owner_officer_id offset mismatch");

// 에디터 UI 및 비즈니스 로직용 DTO
struct ItemRecord {
    uint16_t id = 0;
    std::string name;
    std::string description;

    uint8_t category = 0;
    std::string category_name;

    uint8_t stat_type = 0;
    uint8_t stat_boost = 0;
    std::string stat_boost_desc; // 예: "무력 +8", "통솔 +2", "-"

    uint8_t skill_id = 0xFF;
    uint8_t skill_level = 0;
    std::string skill_desc;      // 예: "일기 Lv.1", "위풍 Lv.1", "-"

    uint8_t horse_speed = 0;     // 명마 기동 보정

    uint16_t value = 0;          // 가치
    uint16_t price = 0;          // 가격

    uint16_t owner_id = 0xFFFF;  // 0xFFFF = 미소유
    std::string owner_name;      // 소유 무장 이름 또는 "미소유/시장"
    bool is_owned = false;

    uintptr_t memory_address = 0;
};

} // namespace San13::Game
