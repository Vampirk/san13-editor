#pragma once

#include "game/CityManager.hpp"
#include "game/SettlementManager.hpp"
#include <string>
#include <vector>

namespace San13::UI {

/**
 * @brief 4탭 도시 종합 인스펙터 모달 (Phase 3)
 * Tab 1: 기본 경제/자원 (Gold, Provisions, Troops, Durability, Pop stats, Cheats)
 * Tab 2: 내정 및 관할 부락 (Commerce, Agriculture, Culture, Public Order, Settlement List)
 * Tab 3: 군사 및 숙련도 (Spear, Horse, Bow proficiencies, City Wall Durability)
 * Tab 4: 도시 소속 인물 명단 (Active, Free, Prisoners tables with full stats)
 */
class CityDetailModal {
public:
    CityDetailModal();
    ~CityDetailModal();

    void Initialize(Game::CityManager* cityMgr, Game::SettlementManager* setMgr = nullptr);

    void Open(int cityId);
    void Close();
    bool IsOpen() const { return m_isOpen; }

    void Render();

    // Hot-Sync and Cheats
    bool CommitEditToMemory();
    bool QuickCheatMaxResources();
    bool QuickCheatDepleteResources();
    bool QuickCheatMaxDomestic();
    bool QuickCheatMaxSettlements();
    bool QuickCheatFortifyAllSettlements();
    bool QuickCheatMaxProficiencies();

    void SetLimitBreakMode(bool enabled) { m_limitBreakMode = enabled; }
    bool IsLimitBreakMode() const { return m_limitBreakMode; }

    void SetAutoSync(bool enabled) { m_autoSync = enabled; }
    bool IsAutoSync() const { return m_autoSync; }

    int GetCityId() const { return m_cityId; }
    Game::CityRecord& GetEditingRecord() { return m_record; }
    const Game::CityRecord& GetEditingRecord() const { return m_record; }

    std::vector<Game::SettlementRecord>& GetSettlements() { return m_settlements; }
    const std::vector<Game::SettlementRecord>& GetSettlements() const { return m_settlements; }

    float GetLastSyncTimeMs() const { return m_lastSyncTimeMs; }
    const std::string& GetSyncStatusMsg() const { return m_syncStatusMsg; }

    void RefreshData();

private:
    void RenderTabEconomy();
    void RenderTabDomestic();
    void RenderTabMilitary();
    void RenderTabOfficers();

    Game::CityManager*                  m_cityMgr = nullptr;
    Game::SettlementManager*            m_setMgr = nullptr;

    bool                                m_isOpen = false;
    int                                 m_cityId = -1;
    Game::CityRecord                    m_record;

    // Settlement data cache
    std::vector<Game::SettlementRecord> m_settlements;

    // City Officers cache
    std::vector<Game::CityOfficerEntry> m_activeOfficers;
    std::vector<Game::CityOfficerEntry> m_freeOfficers;
    std::vector<Game::CityOfficerEntry> m_prisoners;

    bool                                m_limitBreakMode = false;
    bool                                m_autoSync = true;
    std::string                         m_syncStatusMsg;
    float                               m_lastSyncTimeMs = 0.0f;
};

} // namespace San13::UI
