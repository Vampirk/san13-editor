#pragma once

#include "core/IMemoryDriver.hpp"
#include "game/City.hpp"
#include "game/Bounds.hpp"
#include <vector>

namespace San13::Game {

/**
 * @brief 60개 도시 자원 및 데이터 읽기/쓰기를 총괄하는 도메인 매니저
 */
class CityManager {
public:
    CityManager();
    ~CityManager();

    /**
     * @brief 매니저 초기화 및 루트 주소 바인딩
     */
    bool Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase);

    /**
     * @brief 특정 도시 ID의 데이터 로드
     */
    bool LoadCity(uint8_t id, CityRecord& outRecord);

    /**
     * @brief 60개 전체 도시 데이터 일괄 파싱 및 로드
     */
    bool LoadAllCities(std::vector<CityRecord>& outRecords);

    /**
     * @brief 개별 도시 자원 수정 헬퍼 (BoundsValidator 자동 연동)
     */
    bool SetCityGold(uint8_t id, int64_t gold, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityProvisions(uint8_t id, int64_t provisions, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityTroops(uint8_t id, int64_t troops, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityDurability(uint8_t id, int64_t durability, ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 원클릭 치트: 특정 도시 자원 풀충전 (자금 999,999 / 군량 999,999 / 내구도 풀)
     */
    bool MaximizeCityResources(uint8_t id);

    /**
     * @brief 특정 도시의 메모리 주소 산출 공식
     */
    uintptr_t GetCityAddress(uint8_t id) const;

private:
    Core::IMemoryDriver* m_driver;
    uintptr_t m_rootBase;
    uintptr_t m_cityArrayBase;
};

} // namespace San13::Game
