#pragma once

#include "game/CityManager.hpp"
#include "game/SettlementManager.hpp"
#include "ui/CityDetailModal.hpp"
#include "ui/HangulAutomata.hpp"
#include <vector>
#include <string>

namespace San13::UI {

/**
 * @brief 60개 도시 자원 그리드 뷰 및 실시간 자원 편집기 (Phase 5.4 & Phase 3)
 */
class CityTableView {
public:
    CityTableView();
    ~CityTableView();

    /**
     * @brief CityManager 및 SettlementManager 바인딩 및 60개 도시 초기 데이터 로드
     */
    void Initialize(Game::CityManager* cityMgr, Game::SettlementManager* setMgr = nullptr);

    /**
     * @brief 실시간 메모리 데이터 재로드 (새로고침)
     */
    void RefreshData();

    /**
     * @brief ImGui 테이블 렌더링 루틴 (다중 정렬 + 실시간 검색 + 자원 편집)
     */
    void Render();

    /**
     * @brief 현재 필터링된 도시 수 반환
     */
    size_t GetFilteredCount() const { return m_filteredIndices.size(); }
    size_t GetTotalCount() const { return m_cachedCities.size(); }

    /**
     * @brief 선택된 도시 ID 반환 (없을 시 -1)
     */
    int GetSelectedCityId() const { return m_selectedCityId; }
    void SetSelectedCityId(int id) { m_selectedCityId = id; }

    /**
     * @brief 프로그래밍적 검색어 설정 (테스트 및 단축키용)
     */
    void SetSearchQuery(const char* query);

    /**
     * @brief 특정 열 기준 정렬 실행
     */
    void SortByColumn(int columnId, bool ascending);

    /**
     * @brief 필터링된 결과 목록 중 특정 위치의 도시 레코드 반환
     */
    const Game::CityRecord* GetCityAt(size_t filteredIndex) const;

    /**
     * @brief 캐시된 전체 도시 목록 참조
     */
    const std::vector<Game::CityRecord>& GetCachedCities() const { return m_cachedCities; }

    // --- Phase 5.4 도시 자원 편집 & 핫싱크(Hot-Sync) 메서드 ---

    /**
     * @brief 도시 상세 4탭 종합 인스펙터 모달 객체 참조
     */
    CityDetailModal& GetDetailModal() { return m_detailModal; }
    const CityDetailModal& GetDetailModal() const { return m_detailModal; }

    /**
     * @brief 도시 상세 4탭 편집 모달 열기
     */
    void OpenEditModal(int cityId);
    bool IsEditModalOpen() const;
    void CloseEditModal();

    /**
     * @brief 현재 편집 중인 도시 자원 게임 메모리에 즉시 커밋 (핫싱크)
     */
    bool CommitEditToMemory();

    /**
     * @brief 원클릭 치트: 특정 도시 자원 풀충전 (자금 999,999 / 군량 999,999 / 내구 5,000)
     */
    bool QuickCheatMaxResources(uint8_t cityId);

    /**
     * @brief 원클릭 치트: 특정 도시 자원 고갈 (공략용: 자금 0 / 군량 0)
     */
    bool QuickCheatDepleteResources(uint8_t cityId);

    /**
     * @brief 원클릭 치트: 60개 전 도시 자원 풀충전
     */
    bool QuickCheatAllCitiesMax();

    /**
     * @brief 한계 돌파(LimitBreak) 모드 설정/조회
     */
    void SetLimitBreakMode(bool enabled);
    bool IsLimitBreakMode() const;

    /**
     * @brief 실시간 자동 동기화(Live Auto-Sync) 설정/조회
     */
    void SetAutoSync(bool enabled);
    bool IsAutoSync() const;

    /**
     * @brief 편집용 임시 도시 레코드 참조
     */
    Game::CityRecord& GetEditRecord();
    const Game::CityRecord& GetEditRecord() const;
    int GetEditingCityId() const;
    float GetLastSyncTimeMs() const;

private:
    void ApplyFilter();
    void ApplySort(int columnId, bool ascending);
    void RenderEditModal();

    Game::CityManager*               m_cityMgr = nullptr;
    Game::SettlementManager*         m_setMgr = nullptr;
    std::vector<Game::CityRecord>    m_cachedCities;
    std::vector<size_t>              m_filteredIndices;

    char                             m_searchQuery[64] = {0};
    HangulAutomata                   m_searchAutomata;
    int                              m_selectedCityId = -1;

    // 정렬 상태 캐시
    int                              m_lastSortColumn = -1;
    bool                             m_lastSortAscending = true;

    // 4탭 도시 종합 인스펙터 모달 (Phase 3)
    CityDetailModal                  m_detailModal;
};

} // namespace San13::UI
