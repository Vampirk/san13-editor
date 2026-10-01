#pragma once

#include <cstdint>
#include <string>

namespace San13::Game {

#pragma pack(push, 1)

/**
 * @brief 삼국지 13 PK 도시(City) 오브젝트 메모리 레이아웃 (368 바이트 = 0x170 Stride)
 * 실시간 Proton 라이브 메모리 실측을 통해 오프셋 검증 완료.
 */
struct San13CityRaw {
    uint8_t  head_pad[0x58];  // +0x00 ~ +0x57: VTable(0x10), 세력/군주/태수 소속 정보
    uint32_t durability;      // +0x58: 도시 내구도 (낙양 3,000, 업 2,800, 양평 2,200)
    uint32_t gold;            // +0x5C: 자금 (낙양 28,000, 업 24,000, 양평 4,240)
    uint32_t provisions;      // +0x60: 군량 (낙양 125,700, 업 108,000, 양평 95,000)
    uint32_t troops;          // +0x64: 주둔 모병/병력 (낙양 14,000, 업 12,000, 양평 2,120)
    uint8_t  tail_pad[0x170 - 0x68]; // +0x68 ~ +0x16F: 인구, 민심, 상업, 농업 등 기술치
};

#pragma pack(pop)

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
    uint32_t gold = 0;
    uint32_t provisions = 0;
    uint32_t troops = 0;
    uint32_t durability = 0;
    uintptr_t memory_address = 0;
};

/**
 * @brief 삼국지 13 PK 60개 도시 한글 명칭 헬퍼
 */
const char* GetCityName(uint8_t id);

} // namespace San13::Game
