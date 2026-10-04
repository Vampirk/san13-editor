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
     * @brief 특정 도시의 태수/군주 무장 ID 조회 (+0x3E, 0xFFFF=공백지)
     */
    uint16_t GetCityRulerId(uint8_t cityId);

    /**
     * @brief 특정 도시의 소속 무장 연결 리스트 순회 및 로드
     * @param cityId 도시 ID (0 ~ 59)
     * @param category 무장 분류 (Active, Prisoner, Free)
     * @param outOfficers 추출된 무장 정보 목록
     * @return 로드 성공 여부
     */
    bool LoadCityOfficers(uint8_t cityId, CityOfficerCategory category, std::vector<CityOfficerEntry>& outOfficers);

    /**
     * @brief 특정 도시의 모든 카테고리(현역, 포로, 재야) 무장 일괄 로드
     */
    bool LoadAllCityOfficers(uint8_t cityId,
                             std::vector<CityOfficerEntry>& outActive,
                             std::vector<CityOfficerEntry>& outPrisoners,
                             std::vector<CityOfficerEntry>& outFree);

    /**
     * @brief 개별 도시 자원 수정 헬퍼 (BoundsValidator 자동 연동)
     */
    bool SetCityGold(uint8_t id, int64_t gold, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityProvisions(uint8_t id, int64_t provisions, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityTroops(uint8_t id, int64_t troops, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityDurability(uint8_t id, int64_t durability, ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 도시 내정 수치 수정 헬퍼 (BoundsValidator 자동 연동)
     */
    bool SetCityPublicOrder(uint8_t id, int64_t publicOrder, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityCommerce(uint8_t id, int64_t commerce, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityAgriculture(uint8_t id, int64_t agriculture, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityCulture(uint8_t id, int64_t culture, ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 도시 병종 숙련도 수정 헬퍼 (BoundsValidator 자동 연동)
     */
    bool SetCitySpearProficiency(uint8_t id, int64_t spearProf, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityHorseProficiency(uint8_t id, int64_t horseProf, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetCityBowProficiency(uint8_t id, int64_t bowProf, ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 원클릭 치트: 특정 도시 자원 풀충전 (자금 999,999 / 군량 999,999 / 내구도 풀)
     */
    bool MaximizeCityResources(uint8_t id);

    /**
     * @brief 원클릭 치트: 특정 도시 내정치 올 3,000 완충 (민심 1,000 / 상업 3,000 / 농업 3,000 / 문화 3,000)
     */
    bool MaximizeCityDomestic(uint8_t id);

    /**
     * @brief 원클릭 치트: 특정 도시 병종 3대 숙련도 3,000 완충 (고급 병종 즉시 편성 가능)
     */
    bool MaximizeCityProficiency(uint8_t id);

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
