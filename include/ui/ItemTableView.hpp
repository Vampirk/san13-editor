#pragma once

#include "game/ItemManager.hpp"
#include "game/OfficerManager.hpp"
#include "ui/HangulAutomata.hpp"
#include <vector>
#include <string>

namespace San13::UI {

enum class ItemCategoryFilter {
    All = 0,
    Weapon,     // 검, 도, 대도, 창/모, 극, 부, 편, 추, 암기, 궁
    Horse,      // 명마
    Book,       // 병서, 경서, 사서, 자서, 문집, 의서, 정서, 지도
    Treasure,   // 보물, 골동품, 옥새
    MedicineWine,// 약재, 주류
    Other
};

enum class ItemOwnershipFilter {
    All = 0,
    OwnedOnly,
    UnownedOnly
};

class ItemTableView {
public:
    ItemTableView() = default;
    ~ItemTableView() = default;

    void Initialize(Game::ItemManager* itemMgr, Game::OfficerManager* officerMgr);
    void Render();
    void RefreshData();

private:
    void RenderToolbar();
    void RenderItemTable();
    void RenderAssignModal();

    bool MatchesFilter(const Game::ItemRecord& item) const;

    Game::ItemManager* m_itemMgr = nullptr;
    Game::OfficerManager* m_officerMgr = nullptr;

    std::vector<Game::ItemRecord> m_items;
    bool m_needsRefresh = true;

    // 필터 상태
    char m_searchBuffer[128] = {0};
    HangulAutomata m_searchAutomata;
    ItemCategoryFilter m_catFilter = ItemCategoryFilter::All;
    ItemOwnershipFilter m_ownerFilter = ItemOwnershipFilter::All;
    int m_cityFilter = -1; // -1: 전체, 0~59: 특정 도시 소재지/시장

    // 수여 모달 상태
    bool m_showAssignModal = false;
    uint16_t m_selectedItemId = 0;
    char m_assignSearchBuffer[128] = {0};
    HangulAutomata m_assignAutomata;
    int m_selectedOfficerId = -1;
};

} // namespace San13::UI
