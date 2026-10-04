#pragma once

#include "core/IMemoryDriver.hpp"
#include "game/Settlement.hpp"
#include <vector>

namespace San13::Game {

/**
 * @brief 300개 부락(Settlement) 데이터 읽기 및 도시 연동을 총괄하는 도메인 매니저
 */
class SettlementManager {
public:
    SettlementManager();
    ~SettlementManager();

    /**
     * @brief 매니저 초기화 및 루트 주소 바인딩
     */
    bool Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase);

    /**
     * @brief 특정 부락 ID (0 ~ 299) 데이터 로드 (도시 컨텍스트 옵션)
     */
    bool LoadSettlement(uint16_t id, SettlementRecord& outRecord, uint8_t cityId = 255, const std::string& cityName = "");

    /**
     * @brief 300개 전체 부락 데이터 일괄 로드
     */
    bool LoadAllSettlements(std::vector<SettlementRecord>& outRecords);

    /**
     * @brief 특정 도시의 관할 부락 목록 로드 (부락 connected_cities 기반 역방향 탐색)
     * @param cityId 도시 ID (0 ~ 59)
     * @param cityName 도시 한글 명칭
     * @param outRecords 결과 부락 레코드 목록
     */
    bool LoadCitySettlements(uint8_t cityId, const std::string& cityName, std::vector<SettlementRecord>& outRecords);

    /**
     * @brief 기존 호환용 LoadCitySettlements (도시 ID 기반 자동 리다이렉트)
     */
    bool LoadCitySettlements(uint8_t cityId, const uint8_t legacyIds[4], uint32_t count, std::vector<SettlementRecord>& outRecords);

    /**
     * @brief 특정 부락의 메모리 주소 산출
     */
    uintptr_t GetSettlementAddress(uint16_t id) const;

    /**
     * @brief 부락 장악도/개발도 수정 (0 ~ 100%)
     * @param id 부락 ID (0 ~ 299)
     * @param devProgress 장악도 수치
     * @param cityId 대상 도시 ID (255면 주 관할 도시 connected_cities[0] 기준)
     */
    bool SetSettlementDevProgress(uint16_t id, int64_t devProgress, uint8_t cityId = 255);

    /**
     * @brief 원클릭 치트: 특정 도시의 모든 관할 부락 100% 장악 완료
     */
    bool MaximizeCitySettlements(uint8_t cityId);

    /**
     * @brief 기존 호환용 MaximizeCitySettlements
     */
    bool MaximizeCitySettlements(const uint8_t settlementIds[4], uint32_t count);

    /**
     * @brief 부락에 성채(건물)를 올리거나 전환 및 내구도/지배 도시 설정
     * @param id 부락 ID (0 ~ 299)
     * @param type 개축할 성채(건물) 유형 (1:견진, 2:군악대, 3:요양사, 4:병량고, 5:방채)
     * @param hp 설정할 내구도 (0이면 유형별 공식 기본값 자동 지정)
     * @param cityId 주 관할 도시 ID (255면 연결된 주 도시 기준, 지정 시 primary_city_id에 즉시 바인딩)
     */
    bool FortifySettlement(uint16_t id, FortressType type, uint16_t hp = 0, uint8_t cityId = 255);

    /**
     * @brief 부락의 최대 건물 레벨(요충 레벨 1~5) 설정 (승급 치트 지원)
     */
    bool SetSettlementMaxLevel(uint16_t id, uint8_t maxLevel);

    /**
     * @brief 부락 ID와 기본 효과 ID에 따른 원작 공식 기본 최대 건물 레벨(1~5) 산출
     */
    static uint8_t GetDefaultMaxLevel(uint16_t id, uint16_t defaultEffectId);

    /**
     * @brief 요새를 해체하고 일반 부락으로 원복
     */
    bool DemolishFortress(uint16_t id);

    /**
     * @brief 특정 도시의 모든 관할 부락을 일괄 방채(Lv5) 또는 지정 성채로 요새화 치트
     */
    bool FortifyAllCitySettlements(uint8_t cityId, FortressType type = FortressType::Stronghold);

    /**
     * @brief 성채(건물) 유형 한글 명칭 반환 (예: "방채 (防砦)", "견진 (堅陣)")
     */
    static const char* GetFortressTypeName(FortressType type);

    /**
     * @brief UI 뱃지용 간략 태그 반환 (예: "[방채]", "[견진]", "[군악대]")
     */
    static const char* GetFortressTag(FortressType type);

    /**
     * @brief 특정 부락의 CChiiki (PK 요충/성채) 메모리 주소 산출
     */
    uintptr_t GetChiikiAddress(uint16_t id) const;

private:
    Core::IMemoryDriver* m_driver;
    uintptr_t m_rootBase;
    uintptr_t m_settlementArrayBase;
    uintptr_t m_chiikiArrayBase;
};

} // namespace San13::Game
