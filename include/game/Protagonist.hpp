#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace San13::Game {

#pragma pack(push, 1)

/**
 * @brief 삼국지 13 PK 플레이어(주인공) 컨트롤러 메모리 레이아웃
 * 위치: RootBase + 0x0014BC (실측 확정)
 */
struct San13ProtagonistRaw {
    uint32_t vtable;            // +0x00: VTable 포인터 (모듈 베이스 + 0xD69C04, 절대 주소는 실행마다 변동)
    uint32_t officer_id;        // +0x04: 플레이어 무장 고유 ID (0 ~ 2164)
    uint32_t personal_gold;     // +0x08: 주인공 개인 소지 금 (화면 실측 2,000금 100% 일치)
    uint32_t pad_0c;            // +0x0C
};

#pragma pack(pop)

static_assert(sizeof(San13ProtagonistRaw) == 16, "San13ProtagonistRaw size mismatch");
static_assert(offsetof(San13ProtagonistRaw, vtable) == 0x00, "Protagonist vtable offset mismatch");
static_assert(offsetof(San13ProtagonistRaw, officer_id) == 0x04, "Protagonist officer_id offset mismatch");
static_assert(offsetof(San13ProtagonistRaw, personal_gold) == 0x08, "Protagonist personal_gold offset mismatch");

struct ProtagonistMemoryLayout {
    static constexpr uintptr_t CHUNK_OFFSET = 0x0014BC;
    static constexpr uint32_t  VTABLE_RVA = 0x00D69C04;     // 모듈 상대 주소 (1.0.2, EDGE-56)
};

/**
 * @brief UI 및 비즈니스 로직용 주인공 요약 레코드
 */
struct ProtagonistRecord {
    bool        is_valid = false;
    uint16_t    officer_id = 0xFFFF;
    std::string name;
    uint32_t    personal_gold = 0;
    std::string force_name;
    std::string city_name;
    std::string rank_desc;
    uint8_t     leadership = 0;
    uint8_t     war = 0;
    uint8_t     intel = 0;
    uint8_t     politics = 0;
};

} // namespace San13::Game
