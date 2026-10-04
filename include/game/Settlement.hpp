#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace San13::Game {

#pragma pack(push, 1)

/**
 * @brief 삼국지 13 PK 부락(集落, Settlement) 오브젝트 메모리 레이아웃 (28 바이트 = 0x1C Stride)
 * RootBase + 0x00CA48, 총 300개 (ID 0 ~ 299)
 */
struct San13SettlementRaw {
    uint32_t vtable;                   // +0x00: VTable 포인터 (0x72919AC4) [확정]
    uint16_t settlement_id;            // +0x04: 부락 고유 ID (0 ~ 299) [확정]
    uint16_t default_effect_id;        // +0x06: 부락 기본 효과/타입 ID (0:상업소, 3:농업소, 11:호표기 등) [확정]
    uint16_t global_site_id;           // +0x08: 글로벌 거점 ID (60 + settlement_id) [확정]
    uint8_t  connected_cities[4];      // +0x0A ~ +0x0D: 관할 및 인접 도시 ID 목록 (255=미연결) [확정]
    uint8_t  buff_effects[4];          // +0x0E ~ +0x11: 부락 효과/버프 ID 목록 (255=없음) [확정]
    uint8_t  is_active;                // +0x12: 활성 상태 플래그 (1=활성) [확정]
    uint8_t  primary_city_id;          // +0x13: 주 관할 도시 ID [확정]
    uint8_t  city_dev_progress[4];     // +0x14 ~ +0x17: 연결 도시별 장악도 (각각 0 ~ 100%) [확정]
    uint8_t  tail_pad[4];              // +0x18 ~ +0x1B: 청크 말미 패딩 (4B)
};

#pragma pack(pop)

static_assert(sizeof(San13SettlementRaw) == 0x1C, "San13SettlementRaw size must be exactly 28 bytes");
static_assert(offsetof(San13SettlementRaw, vtable) == 0x00, "vtable offset mismatch");
static_assert(offsetof(San13SettlementRaw, settlement_id) == 0x04, "settlement_id offset mismatch");
static_assert(offsetof(San13SettlementRaw, default_effect_id) == 0x06, "default_effect_id offset mismatch");
static_assert(offsetof(San13SettlementRaw, global_site_id) == 0x08, "global_site_id offset mismatch");
static_assert(offsetof(San13SettlementRaw, connected_cities) == 0x0A, "connected_cities offset mismatch");
static_assert(offsetof(San13SettlementRaw, buff_effects) == 0x0E, "buff_effects offset mismatch");
static_assert(offsetof(San13SettlementRaw, is_active) == 0x12, "is_active offset mismatch");
static_assert(offsetof(San13SettlementRaw, primary_city_id) == 0x13, "primary_city_id offset mismatch");
static_assert(offsetof(San13SettlementRaw, city_dev_progress) == 0x14, "city_dev_progress offset mismatch");

struct SettlementMemoryLayout {
    static constexpr uintptr_t CHUNK_OFFSET = 0x00CA48;        // RootBase로부터의 부락 청크 오프셋
    static constexpr uintptr_t CHIIKI_CHUNK_OFFSET = 0xD31A10; // RootBase로부터의 CChiiki (PK 요충/성채) 청크 오프셋
    static constexpr size_t    OBJECT_STRIDE = 0x1C;           // 단일 부락 크기 (28 바이트)
    static constexpr size_t    CHIIKI_STRIDE = 0x14;           // CChiiki 크기 (20 바이트)
    static constexpr uint16_t  MAX_SETTLEMENT_COUNT = 300;     // 삼국지 13 PK 총 부락 수
};

/**
 * @brief 부락 기본 분류 (고정 맵 부락 vs PK 동적 요새 슬롯)
 */
enum class SettlementKind : uint8_t {
    FixedSettlement = 0,    // 0 ~ 146: 고정 맵 부락 (147개)
    DynamicFortress = 1     // 147 ~ 299: 동적 요새/진 슬롯 (153개)
};

/**
 * @brief 삼국지 13 PK 요충 개축 / 성채(건물) 유형 (공식 5대 성채)
 */
enum class FortressType : uint8_t {
    None = 0,               // 일반 부락 (미건축)
    Camp = 1,               // 견진 (堅陣, 필요 Lv 1, 내구 1000)
    MusicTower = 2,         // 군악대 (軍樂臺, 필요 Lv 2, 내구 1500)
    Hospital = 3,           // 요양사 (療養舎, 필요 Lv 3, 내구 1500)
    SupplyDepot = 4,        // 병량고 (兵糧庫, 필요 Lv 4, 내구 2000)
    Stronghold = 5          // 방채 (防砦, 필요 Lv 5, 내구 2500)
};

/**
 * @brief UI 및 비즈니스 로직용 부락 레코드
 */
struct SettlementRecord {
    uint16_t id = 0;
    std::string name;             // 부락 명칭 ("범", "밀", "온" 등 인게임 지명)
    std::string effect_name;      // 부락 효과 명칭 ("호표기", "인구+20000" 등)
    uint16_t default_effect_id = 255; // 부락 기본 효과 ID
    uint16_t force_id = 0xFFFF;   // 지배 세력 ID (0xFFFF=미종속/중립)
    std::string force_name = "미종속 (중립)"; // 지배 세력명 ("조조군", "미종속 (중립)")
    uint16_t dev_progress = 0;    // 장악도 (0 ~ 100%)
    uint8_t  city_id = 255;       // 관할 주 도시 ID
    uint8_t  connected_cities[4] = {255, 255, 255, 255}; // 연결 도시들
    uint8_t  buff_effects[4] = {255, 255, 255, 255};     // 효과 ID들
    uintptr_t memory_address = 0;

    // Phase 4: 요새/성채(건물) 및 요충 레벨 체계
    SettlementKind kind = SettlementKind::FixedSettlement;
    uint8_t      max_fortress_level = 1; // 부락 최대 건물 레벨 (Lv 1 ~ 5)
    uint8_t      current_fortress_level = 0; // 현재 지어진 건물 레벨 (0=미건축)
    FortressType fortress_type = FortressType::None;
    std::string  type_tag = "[부락]"; // "[부락]", "[견진]", "[군악대]", "[요양사]", "[병량고]", "[방채]"
    uint16_t     hp = 0;             // 건물 내구도
    uint16_t     max_hp = 0;         // 건물 최대 내구도
    bool         is_fortified = false; // 건물 건축 여부
};

} // namespace San13::Game
