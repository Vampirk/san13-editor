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
    uint8_t  head_pad[0x18];          // +0x00 ~ +0x17: VTable(0x10), 세력/군주/태수 소속 정보
    uint8_t  connected_city_ids[4];   // +0x018 ~ +0x01B: 도로망으로 직결된 인접 도시 ID 목록 (구 settlement_ids 오독 정정) [확정]
    uint8_t  head_pad2[0x22];         // +0x01C ~ +0x03D: 지역/좌표 정보 (34B)
    uint16_t ruler_id;                // +0x03E: 도시 태수/군주 무장 ID (0xFFFF=공백지) [확정]
    uint8_t  head_pad3[0x18];         // +0x040 ~ +0x057: 기타 헤더 정보 (24B)
    uint32_t gold;                    // +0x058: 도시 자금 [확정] (낙양 4,000, 양평 2,200)
    uint32_t provisions;              // +0x05C: 도시 군량 [확정] (낙양 44,400, 양평 4,240)
    uint32_t civilian_population;     // +0x060: 기초 민간 인구 [확정] (낙양 200,000, 양평 95,000)
    uint32_t troops;                  // +0x064: 주둔 병력 [확정] (낙양 22,200, 양평 2,120)
    uint8_t  pad_0x68[0x6A];          // +0x068 ~ +0x0D1: 부상병/귀환병 후보 영역 (106B)
    uint16_t public_order;            // +0x0D2: 민심 (양평 100, 낙양 350 실측) [확정]
    uint8_t  pad_0xD4[2];             // +0x0D4 ~ +0x0D5: 패딩 (2B)
    uint16_t commerce;                // +0x0D6: 상업 기본 개발치 (양평 1000, 낙양 1800 실측) [유력]
    uint8_t  pad_0xD8[8];             // +0x0D8 ~ +0x0DF: 패딩 (8B)
    uint16_t agriculture;             // +0x0E0: 농업 기본 개발치 (양평 1050, 낙양 2000 실측) [확정]
    uint8_t  pad_0xE2[8];             // +0x0E2 ~ +0x0E9: 패딩 (8B)
    uint16_t culture;                 // +0x0EA: 문화 기본 개발치 (양평 1000, 낙양 2400 실측) [확정]
    uint8_t  pad_0xEC[0x10];          // +0x0EC ~ +0x0FB: 패딩 (16B)
    uint16_t spear_proficiency;       // +0x0FC: 창병 숙련도 (양평 200, 낙양 700 실측) [확정]
    uint16_t horse_proficiency;       // +0x0FE: 기병 숙련도 (양평 300, 낙양 700 실측) [확정]
    uint16_t bow_proficiency;         // +0x100: 궁병 숙련도 (양평 200, 낙양 700 실측) [확정]
    uint8_t  pad_0x102[0x3A];         // +0x102 ~ +0x13B: 내구도/방어시설 후보 영역 (58B)
    uint32_t settlement_count;        // +0x13C: 관할 부락 수 [확정] (양평 1, 낙양 4)
    uint32_t officer_list_ptr;        // +0x140: 소속 무장 링크드 리스트 헤드 포인터 [확정]
    uint32_t officer_count;           // +0x144: 소속 무장 수 [유력] (낙양 12명 실측)
    uint32_t prisoner_list_ptr;       // +0x148: 수감 포로 링크드 리스트 헤드 포인터
    uint32_t prisoner_count;          // +0x14C: 수감 포로 수 [유력] (낙양 0명 실측)
    uint32_t free_officer_list_ptr;   // +0x150: 재야 무장 링크드 리스트 헤드 포인터
    uint32_t free_officer_count;      // +0x154: 재야 무장 수 [유력] (낙양 1명 실측)
    uint8_t  tail_pad[0x18];          // +0x158 ~ +0x16F: 청크 말미 패딩 (24B)
};

#pragma pack(pop)

static_assert(sizeof(San13CityRaw) == 0x170, "San13CityRaw size must be exactly 0x170 (368 bytes)");
static_assert(offsetof(San13CityRaw, connected_city_ids) == 0x18, "connected_city_ids offset mismatch");
static_assert(offsetof(San13CityRaw, ruler_id) == 0x3E, "ruler_id offset mismatch");
static_assert(offsetof(San13CityRaw, gold) == 0x58, "gold offset mismatch");
static_assert(offsetof(San13CityRaw, provisions) == 0x5C, "provisions offset mismatch");
static_assert(offsetof(San13CityRaw, civilian_population) == 0x60, "civilian_population offset mismatch");
static_assert(offsetof(San13CityRaw, troops) == 0x64, "troops offset mismatch");
static_assert(offsetof(San13CityRaw, public_order) == 0xD2, "public_order offset mismatch");
static_assert(offsetof(San13CityRaw, commerce) == 0xD6, "commerce offset mismatch");
static_assert(offsetof(San13CityRaw, agriculture) == 0xE0, "agriculture offset mismatch");
static_assert(offsetof(San13CityRaw, culture) == 0xEA, "culture offset mismatch");
static_assert(offsetof(San13CityRaw, spear_proficiency) == 0xFC, "spear_proficiency offset mismatch");
static_assert(offsetof(San13CityRaw, horse_proficiency) == 0xFE, "horse_proficiency offset mismatch");
static_assert(offsetof(San13CityRaw, bow_proficiency) == 0x100, "bow_proficiency offset mismatch");
static_assert(offsetof(San13CityRaw, settlement_count) == 0x13C, "settlement_count offset mismatch");
static_assert(offsetof(San13CityRaw, officer_list_ptr) == 0x140, "officer_list_ptr offset mismatch");
static_assert(offsetof(San13CityRaw, officer_count) == 0x144, "officer_count offset mismatch");
static_assert(offsetof(San13CityRaw, prisoner_list_ptr) == 0x148, "prisoner_list_ptr offset mismatch");
static_assert(offsetof(San13CityRaw, prisoner_count) == 0x14C, "prisoner_count offset mismatch");
static_assert(offsetof(San13CityRaw, free_officer_list_ptr) == 0x150, "free_officer_list_ptr offset mismatch");
static_assert(offsetof(San13CityRaw, free_officer_count) == 0x154, "free_officer_count offset mismatch");

#pragma pack(push, 1)

/**
 * @brief 삼국지 13 PK 도시 소속 무장 연결 리스트 노드 구조체 (12 바이트)
 * City +0x140(현역), +0x148(포로), +0x150(재야)의 std::list 순회용 노드
 */
struct San13CityOfficerNodeRaw {
    uint32_t next_node;        // +0x00: 다음 노드 주소
    uint32_t prev_node;        // +0x04: 이전 노드 주소
    uint32_t officer_obj_ptr;  // +0x08: 무장 풀 오브젝트 가상 주소
};

#pragma pack(pop)

static_assert(sizeof(San13CityOfficerNodeRaw) == 12, "San13CityOfficerNodeRaw size must be exactly 12 bytes");
static_assert(offsetof(San13CityOfficerNodeRaw, next_node) == 0x00, "next_node offset mismatch");
static_assert(offsetof(San13CityOfficerNodeRaw, prev_node) == 0x04, "prev_node offset mismatch");
static_assert(offsetof(San13CityOfficerNodeRaw, officer_obj_ptr) == 0x08, "officer_obj_ptr offset mismatch");

enum class CityOfficerCategory {
    Active,    // 현역 소속 (+0x140, count: +0x144)
    Prisoner,  // 수감 포로 (+0x148, count: +0x14C)
    Free       // 재야 무장 (+0x150, count: +0x154)
};

/**
 * @brief 도시 인스펙터 4탭 및 비즈니스 로직용 무장 정보 엔트리
 */
struct CityOfficerEntry {
    uint16_t    officer_id = 0;
    std::string name;
    uint8_t     rank = 0;              // 품관/신분 (0: 군주, 1~9: 1~9품관, 재야 등)
    std::string status_desc;           // "군주", "일반 (5품관)", "재야", "포로" 등
    uint8_t     curr_leadership = 0;
    uint8_t     curr_war = 0;
    uint8_t     curr_intel = 0;
    uint8_t     curr_politics = 0;
    uint8_t     loyalty = 0;
    uint16_t    force_id = 0;
    uintptr_t   officer_addr = 0;
};

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

    // 지배 세력 및 군주 (확정)
    uint16_t ruler_id = 0xFFFF;           // 태수/군주 무장 ID (+0x3E, 0xFFFF=공백지)
    std::string ruler_name;               // 태수/군주 한글 성명 (예: "조조")
    std::string force_name;               // 지배 세력명 (예: "조조군", "공백지 / 무소속")

    // 기본 자원 및 인구 (확정)
    uint32_t gold = 0;                    // 자금 (+0x58)
    uint32_t provisions = 0;              // 군량 (+0x5C)
    uint32_t civilian_population = 0;     // 기초 민간 인구 (+0x60)
    uint32_t troops = 0;                  // 주둔 병력 (+0x64)
    uint32_t total_population = 0;        // 총 인구 (civilian_population + troops)
    uint32_t durability = 0;              // 내구도

    // 관할 부락 및 인접 도로망 정보
    uint32_t settlement_count = 0;        // 관할 부락 수 (+0x13C)
    uint8_t  connected_city_ids[4] = {255, 255, 255, 255}; // 도로망 인접 도시 ID 목록 (+0x018)

    // 내정 4대치 (읽기 전용)
    uint16_t public_order = 0;            // 민심 (+0x0D2)
    uint16_t commerce = 0;                // 상업 기본치 (+0x0D6)
    uint16_t agriculture = 0;             // 농업 기본치 (+0x0E0)
    uint16_t culture = 0;                 // 문화 기본치 (+0x0EA)

    // 병과 3대 숙련도 (읽기 전용)
    uint16_t spear_proficiency = 0;       // 창병 숙련도 (+0x0FC)
    uint16_t horse_proficiency = 0;       // 기병 숙련도 (+0x0FE)
    uint16_t bow_proficiency = 0;         // 궁병 숙련도 (+0x100)

    // 소속 인물 통계 (읽기 전용)
    uint32_t officer_count = 0;           // 소속 무장 수 (+0x144)
    uint32_t prisoner_count = 0;          // 수감 포로 수 (+0x14C)
    uint32_t free_officer_count = 0;      // 재야 무장 수 (+0x154)

    uintptr_t memory_address = 0;

    uint32_t GetTotalPopulation() const {
        return civilian_population + troops;
    }
};

/**
 * @brief 삼국지 13 PK 60개 도시 한글 명칭 헬퍼
 */
const char* GetCityName(uint8_t id);

} // namespace San13::Game
