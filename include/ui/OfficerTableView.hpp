#pragma once

#include "game/OfficerManager.hpp"
#include "ui/HangulAutomata.hpp"
#include <vector>
#include <string>
#include <unordered_set>

namespace San13::UI {

enum class OfficerFilterMode {
    All = 0,         // 전체 무장
    Active = 1,      // 현역 무장 (현재 활동 중 & 소속 세력 있음)
    Free = 2,        // 재야 무장 (현재 활동 중 & 무소속)
    Unappeared = 3,  // 미등장 무장 (시나리오 현재 연도 미도달)
    Extended = 4     // 신무장 / 결혼 자녀 무장 (ID 800+)
};

/**
 * @brief 대용량 800~2,165 무장 가변 그리드 및 고속 가상 스크롤(ImGuiListClipper) 테이블 뷰
 */
class OfficerTableView {
public:
    OfficerTableView();
    ~OfficerTableView();

    /**
     * @brief OfficerManager와 데이터 바인딩 및 초기 무장 목록 로드
     */
    void Initialize(Game::OfficerManager* officerMgr);

    /**
     * @brief 실시간 메모리 데이터 재로드 (새로고침)
     */
    void RefreshData();

    /**
     * @brief ImGui 테이블 렌더링 루틴 (가상 스크롤 + 다중 정렬 + 실시간 검색)
     */
    void Render();

    /**
     * @brief 현재 필터링된 무장 수 반환
     */
    size_t GetFilteredCount() const { return m_filteredIndices.size(); }
    size_t GetTotalCount() const { return m_cachedOfficers.size(); }

    /**
     * @brief [EDGE-25] 현재 게임 날짜 반환
     */
    uint16_t GetGameYear() const { return m_gameYear; }
    uint16_t GetGameMonth() const { return m_gameMonth; }
    uint16_t GetGameDay() const { return m_gameDay; }

    /**
     * @brief 선택된 무장 ID 반환 (없을 시 -1)
     */
    int GetSelectedOfficerId() const { return m_selectedOfficerId; }
    void SetSelectedOfficerId(int id) { m_selectedOfficerId = id; }

    /**
     * @brief 프로그래밍적 검색어 설정 (테스트 및 단축키용)
     */
    void SetSearchQuery(const char* query);

    /**
     * @brief 프로그래밍적 필터 모드 설정
     */
    void SetFilterMode(OfficerFilterMode mode);

    /**
     * @brief 특정 열 기준 정렬 실행
     */
    void SortByColumn(int columnId, bool ascending);

    /**
     * @brief 필터링된 결과 목록 중 특정 위치의 무장 레코드 반환
     */
    const Game::OfficerRecord* GetOfficerAt(size_t filteredIndex) const;

    /**
     * @brief 캐시된 전체 무장 목록 참조
     */
    const std::vector<Game::OfficerRecord>& GetCachedOfficers() const { return m_cachedOfficers; }

    // --- Phase 5.3 인라인 편집 & 핫싱크(Hot-Sync) 메서드 ---

    /**
     * @brief 무장 능력치 상세 편집 모달 열기
     */
    void OpenEditModal(int officerId);
    bool IsEditModalOpen() const { return m_showEditModal; }
    void CloseEditModal() { m_showEditModal = false; }

    /**
     * @brief 현재 편집 중인 무장 스탯 게임 메모리에 즉시 커밋 (핫싱크)
     */
    bool CommitEditToMemory();

    /**
     * @brief 빠른 치트 적용: 능력치 올 100/255, 전종 S급
     */
    bool QuickCheatAll100(uint16_t officerId);
    bool QuickCheatAll255(uint16_t officerId);
    bool QuickCheatAllAptitudeS(uint16_t officerId);

    /**
     * @brief 한계 돌파(LimitBreak) 모드 설정/조회
     */
    void SetLimitBreakMode(bool enabled) { m_limitBreakMode = enabled; }
    bool IsLimitBreakMode() const { return m_limitBreakMode; }

    /**
     * @brief 실시간 자동 동기화(Live Auto-Sync) 설정/조회
     */
    void SetAutoSync(bool enabled) { m_autoSync = enabled; }
    bool IsAutoSync() const { return m_autoSync; }

    /**
     * @brief 편집용 임시 스탯 버퍼 참조
     */
    Game::San13OfficerLiveStats& GetEditStats() { return m_editStats; }
    const Game::San13OfficerLiveStats& GetEditStats() const { return m_editStats; }
    int GetEditingOfficerId() const { return m_editingOfficerId; }
    float GetLastSyncTimeMs() const { return m_lastSyncTimeMs; }

    // --- Phase 5.5 다중 선택(Multi-Select) & 일괄 수정(Batch Actions) ---

    /**
     * @brief 다중 선택 상태 관리
     */
    bool IsOfficerSelected(uint16_t id) const;
    void SelectOfficer(uint16_t id, bool selected);
    void ToggleSelectOfficer(uint16_t id);
    void SelectAllFiltered();
    void ClearSelection();
    void InvertSelection();
    void SelectSameForceAs(uint16_t officerId);
    void SelectByForce(uint16_t forceId);
    void SelectSameCityAs(uint16_t officerId);
    void SelectByCity(uint16_t cityId);
    void SelectByRank(uint8_t rank);
    size_t GetSelectedCount() const { return m_selectedOfficerIds.size(); }
    const std::unordered_set<uint16_t>& GetSelectedOfficerIds() const { return m_selectedOfficerIds; }

    /**
     * @brief 일괄 수정 모달 제어
     */
    void OpenBatchModal();
    bool IsBatchModalOpen() const { return m_showBatchModal; }
    void CloseBatchModal() { m_showBatchModal = false; }

    /**
     * @brief 일괄 수정 비즈니스 로직
     */
    bool ApplyBatchStatsDelta(int dLead, int dWar, int dIntel, int dPol);
    bool ApplyBatchStatsSet(int lead, int war, int intel, int pol, bool setL, bool setW, bool setI, bool setP);
    bool ApplyBatchAptitudes(int spear, int horse, int bow);
    bool ApplyBatchLifeExtension(int addYears);

    /**
     * @brief 빠른 일괄 치트 프리셋
     */
    bool BatchCheatAllBuff10();    // 선택 무장 전원 올스탯 +10 (SafeVanilla)
    bool BatchCheatAll100();       // 선택 무장 전원 올 100
    bool BatchCheatAll255();       // 선택 무장 전원 올 255 (LimitBreak)
    bool BatchCheatAllAptitudeS(); // 선택 무장 전원 전종 S급
    bool BatchCheatLifeExt30();    // 선택 무장 전원 수명 +30년 연장

    /**
     * @brief 트랜잭션 롤백 (실행 취소 / Undo)
     */
    bool UndoLastBatchOperation();
    bool HasUndoSnapshot() const { return !m_lastBatchSnapshots.empty(); }
    size_t GetLastBatchCount() const { return m_lastBatchSnapshots.size(); }
    float GetLastBatchTimeMs() const { return m_lastBatchTimeMs; }
    const std::string& GetBatchStatusMsg() const { return m_batchStatusMsg; }

private:
    void ApplyFilter();
    void ApplySort(int columnId, bool ascending);
    void RenderEditModal();
    void RenderBatchEditModal();

    Game::OfficerManager*            m_officerMgr = nullptr;
    std::vector<Game::OfficerRecord> m_cachedOfficers;
    std::vector<size_t>              m_filteredIndices;

    char                             m_searchQuery[64] = {0};
    HangulAutomata                   m_searchAutomata;
    OfficerFilterMode                m_filterMode = OfficerFilterMode::All;
    int                              m_selectedOfficerId = -1;

    // EDGE-25 시나리오 일자
    uint16_t                         m_gameYear = 0;
    uint16_t                         m_gameMonth = 0;
    uint16_t                         m_gameDay = 0;

    // 정렬 상태 캐시
    int                              m_lastSortColumn = -1;
    bool                             m_lastSortAscending = true;

    // Phase 5.3 편집 모달 및 핫싱크 상태
    bool                             m_showEditModal = false;
    int                              m_editingOfficerId = -1;
    Game::OfficerRecord              m_editingRecord;
    Game::San13OfficerLiveStats      m_editStats{};
    bool                             m_limitBreakMode = false;
    bool                             m_autoSync = true;
    std::string                      m_syncStatusMsg;
    float                            m_lastSyncTimeMs = 0.0f;

    // Phase 5.5 다중 선택 및 일괄 수정 상태
    std::unordered_set<uint16_t>       m_selectedOfficerIds;
    int                                m_lastClickedRow = -1;
    bool                               m_showBatchModal = false;
    std::vector<Game::OfficerSnapshot> m_lastBatchSnapshots;
    float                              m_lastBatchTimeMs = 0.0f;
    std::string                        m_batchStatusMsg;

    // 일괄 수정 버퍼
    int                                m_batchDeltaLead = 10;
    int                                m_batchDeltaWar = 10;
    int                                m_batchDeltaIntel = 10;
    int                                m_batchDeltaPol = 10;

    int                                m_batchSetLead = 100;
    int                                m_batchSetWar = 100;
    int                                m_batchSetIntel = 100;
    int                                m_batchSetPol = 100;
    bool                               m_batchApplyLead = true;
    bool                               m_batchApplyWar = true;
    bool                               m_batchApplyIntel = true;
    bool                               m_batchApplyPol = true;

    int                                m_batchSpearApt = 3;
    int                                m_batchHorseApt = 3;
    int                                m_batchBowApt = 3;

    int                                m_batchLifeYears = 20;
};

} // namespace San13::UI
