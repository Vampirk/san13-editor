#include "ui/ItemTableView.hpp"
#include "ui/FontHelper.hpp"
#include "game/OfficerNameTable.hpp"
#include "game/City.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>

namespace San13::UI {

void ItemTableView::Initialize(Game::ItemManager* itemMgr, Game::OfficerManager* officerMgr) {
    m_itemMgr = itemMgr;
    m_officerMgr = officerMgr;
    m_needsRefresh = true;
}

void ItemTableView::RefreshData() {
    if (m_itemMgr) {
        m_itemMgr->LoadAllItems(m_items);
        m_needsRefresh = false;
    }
}

bool ItemTableView::MatchesFilter(const Game::ItemRecord& item) const {
    // 1. 소유 필터
    if (m_ownerFilter == ItemOwnershipFilter::OwnedOnly && !item.is_owned) return false;
    if (m_ownerFilter == ItemOwnershipFilter::UnownedOnly && item.is_owned) return false;

    // 1-1. 도시 시장 필터
    if (m_cityFilter >= 0) {
        if (item.is_owned || item.location_city_id != static_cast<uint8_t>(m_cityFilter)) return false;
    }

    // 2. 카테고리 필터
    if (m_catFilter != ItemCategoryFilter::All) {
        uint8_t cat = item.category;
        switch (m_catFilter) {
            case ItemCategoryFilter::Weapon:
                if (cat < 1 || cat > 10) return false; // 검~궁
                break;
            case ItemCategoryFilter::Horse:
                if (cat != 0) return false; // 명마
                break;
            case ItemCategoryFilter::Book:
                if (cat < 11 || cat > 18) return false; // 병서~지도
                break;
            case ItemCategoryFilter::Treasure:
                if (cat != 19 && cat != 22 && cat != 23) return false; // 보물, 골동품, 옥새
                break;
            case ItemCategoryFilter::MedicineWine:
                if (cat != 20 && cat != 21) return false; // 주류, 약재
                break;
            case ItemCategoryFilter::Other:
                if (cat <= 23) return false;
                break;
            default:
                break;
        }
    }

    // 3. 검색어 필터
    if (m_searchBuffer[0] != '\0') {
        std::string needle(m_searchBuffer);
        std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

        std::string nameLower = item.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

        std::string ownerLower = item.owner_name;
        std::transform(ownerLower.begin(), ownerLower.end(), ownerLower.begin(), ::tolower);

        std::string catLower = item.category_name;
        std::transform(catLower.begin(), catLower.end(), catLower.begin(), ::tolower);

        std::string locLower = item.location_name;
        std::transform(locLower.begin(), locLower.end(), locLower.begin(), ::tolower);

        if (nameLower.find(needle) == std::string::npos &&
            ownerLower.find(needle) == std::string::npos &&
            catLower.find(needle) == std::string::npos &&
            locLower.find(needle) == std::string::npos) {
            return false;
        }
    }

    return true;
}

void ItemTableView::Render() {
    if (m_needsRefresh) {
        RefreshData();
    }

    RenderToolbar();
    RenderItemTable();
    RenderAssignModal();
}

void ItemTableView::RenderToolbar() {
    ImGui::BeginChild("ItemToolbar", ImVec2(0, 75), true);

    // 1단: 검색 및 새로고침
    ImGui::AlignTextToFramePadding();
    ImGui::Text("명품 검색:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    HangulInputText("##ItemSearch", "명품 또는 소유자 검색...", m_searchBuffer, sizeof(m_searchBuffer), m_searchAutomata);

    ImGui::SameLine();
    if (ImGui::Button("초기화##ClearItemSearch")) {
        m_searchBuffer[0] = '\0';
        m_searchAutomata.Clear();
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    if (ImGui::Button("새로고침 (F5)##RefreshItems")) {
        RefreshData();
    }

    // 통계 표기
    size_t ownedCount = 0;
    for (const auto& it : m_items) {
        if (it.is_owned) ownedCount++;
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), 
                       "| 총 %zu종 (소유: %zu종, 미소유: %zu종)", 
                       m_items.size(), ownedCount, m_items.size() - ownedCount);

    // 2단: 카테고리 및 소유 필터
    ImGui::AlignTextToFramePadding();
    ImGui::Text("분류:");
    ImGui::SameLine();
    auto CatRadio = [&](const char* label, ItemCategoryFilter filter) {
        if (ImGui::RadioButton(label, m_catFilter == filter)) {
            m_catFilter = filter;
        }
        ImGui::SameLine();
    };

    CatRadio("전체##CatAll", ItemCategoryFilter::All);
    CatRadio("무기##CatWep", ItemCategoryFilter::Weapon);
    CatRadio("명마##CatHorse", ItemCategoryFilter::Horse);
    CatRadio("서적##CatBook", ItemCategoryFilter::Book);
    CatRadio("보물##CatTreasure", ItemCategoryFilter::Treasure);
    CatRadio("약재/주류##CatMed", ItemCategoryFilter::MedicineWine);

    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Text("| 소유:");
    ImGui::SameLine();
    if (ImGui::RadioButton("전체##OwnAll", m_ownerFilter == ItemOwnershipFilter::All && m_cityFilter == -1)) {
        m_ownerFilter = ItemOwnershipFilter::All;
        m_cityFilter = -1;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("소유 중##OwnYes", m_ownerFilter == ItemOwnershipFilter::OwnedOnly)) {
        m_ownerFilter = ItemOwnershipFilter::OwnedOnly;
        m_cityFilter = -1;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("미소유(시장)##OwnNo", m_ownerFilter == ItemOwnershipFilter::UnownedOnly && m_cityFilter == -1)) {
        m_ownerFilter = ItemOwnershipFilter::UnownedOnly;
        m_cityFilter = -1;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Text("| 도시 시장:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    const char* curCityTitle = (m_cityFilter >= 0 && m_cityFilter < 60) ? Game::GetCityName(static_cast<uint8_t>(m_cityFilter)) : "전체 도시";
    if (ImGui::BeginCombo("##ItemCityCombo", curCityTitle)) {
        if (ImGui::Selectable("전체 도시", m_cityFilter == -1)) {
            m_cityFilter = -1;
        }
        for (uint8_t cid = 0; cid < 60; cid++) {
            const char* cName = Game::GetCityName(cid);
            if (!cName || cName[0] == '\0') continue;
            char cBuf[64];
            std::snprintf(cBuf, sizeof(cBuf), "%s (#%02u)", cName, cid);
            if (ImGui::Selectable(cBuf, m_cityFilter == cid)) {
                m_cityFilter = cid;
                m_ownerFilter = ItemOwnershipFilter::UnownedOnly;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::EndChild();
}

void ItemTableView::RenderItemTable() {
    std::vector<const Game::ItemRecord*> filtered;
    filtered.reserve(m_items.size());
    for (const auto& it : m_items) {
        if (MatchesFilter(it)) {
            filtered.push_back(&it);
        }
    }

    const ImGuiTableFlags tableFlags = 
        ImGuiTableFlags_Borders | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY | 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_SizingFixedFit;

    if (ImGui::BeginTable("ItemGridTable", 8, tableFlags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 45.0f);
        ImGui::TableSetupColumn("명품 이름", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("분류", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("능력치 보정", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("부가 특기", ImGuiTableColumnFlags_WidthFixed, 95.0f);
        ImGui::TableSetupColumn("가치/가격", ImGuiTableColumnFlags_WidthFixed, 95.0f);
        ImGui::TableSetupColumn("소유자 / 소재지", ImGuiTableColumnFlags_WidthFixed, 155.0f);
        ImGui::TableSetupColumn("관리 / 조작", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        uint16_t protId = m_officerMgr ? m_officerMgr->GetProtagonistOfficerId() : 0xFFFF;

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(filtered.size()));

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                const auto* item = filtered[row];
                ImGui::TableNextRow();

                // 1. ID
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", item->id);

                // 2. 명품 이름
                ImGui::TableSetColumnIndex(1);
                if (item->value >= 50) {
                    ImGui::TextColored(ImVec4(1.00f, 0.85f, 0.25f, 1.0f), "★ %s", item->name.c_str());
                } else if (item->value >= 30) {
                    ImGui::TextColored(ImVec4(0.96f, 0.88f, 0.65f, 1.0f), "%s", item->name.c_str());
                } else {
                    ImGui::TextColored(ImVec4(0.90f, 0.90f, 0.90f, 1.0f), "%s", item->name.c_str());
                }
                if (ImGui::IsItemHovered() && !item->description.empty()) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(1.00f, 0.84f, 0.0f, 1.0f), "【 %s 】", item->name.c_str());
                    ImGui::Separator();
                    ImGui::TextWrapped("%s", item->description.c_str());
                    ImGui::EndTooltip();
                }

                // 3. 분류 (카테고리 뱃지)
                ImGui::TableSetColumnIndex(2);
                ImVec4 catColor(0.85f, 0.85f, 0.85f, 1.0f);
                if (item->category == 0) catColor = ImVec4(0.38f, 0.88f, 0.52f, 1.0f); // 명마: 비취
                else if (item->category >= 1 && item->category <= 10) catColor = ImVec4(0.96f, 0.45f, 0.45f, 1.0f); // 무기: 진홍
                else if (item->category >= 11 && item->category <= 18) catColor = ImVec4(0.40f, 0.75f, 1.00f, 1.0f); // 서적: 사파이어
                else if (item->category == 23) catColor = ImVec4(1.00f, 0.84f, 0.20f, 1.0f); // 옥새: 제왕 황금
                ImGui::TextColored(catColor, "[%s]", item->category_name.c_str());

                // 4. 능력치 보정
                ImGui::TableSetColumnIndex(3);
                if (item->stat_boost > 0 || item->horse_speed > 0) {
                    ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "%s", item->stat_boost_desc.c_str());
                } else {
                    ImGui::TextDisabled("-");
                }

                // 5. 부가 특기
                ImGui::TableSetColumnIndex(4);
                if (item->skill_level > 0) {
                    ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.00f, 1.0f), "%s", item->skill_desc.c_str());
                } else {
                    ImGui::TextDisabled("-");
                }

                // 6. 가치 / 가격
                ImGui::TableSetColumnIndex(5);
                if (item->value >= 50) {
                    ImGui::TextColored(ImVec4(1.00f, 0.85f, 0.25f, 1.0f), "★%u / %u금", item->value, item->price);
                } else if (item->value >= 30) {
                    ImGui::TextColored(ImVec4(0.96f, 0.76f, 0.26f, 1.0f), "%u / %u금", item->value, item->price);
                } else {
                    ImGui::Text("%u / %u금", item->value, item->price);
                }

                // 7. 소유자 / 소재지
                ImGui::TableSetColumnIndex(6);
                if (item->is_owned) {
                    if (item->owner_id == protId) {
                        ImGui::TextColored(ImVec4(1.00f, 0.84f, 0.0f, 1.0f), "★%s (주인공)", item->owner_name.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.90f, 0.90f, 0.90f, 1.0f), "%s (#%u)", item->owner_name.c_str(), item->owner_id);
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.96f, 0.76f, 0.26f, 1.0f), "🛒 %s", item->location_name.c_str());
                }

                // 8. 관리 / 조작
                ImGui::TableSetColumnIndex(7);
                ImGui::PushID(item->id);

                if (item->is_owned) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.20f, 0.20f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
                    if (ImGui::SmallButton("몰수")) {
                        if (m_itemMgr) {
                            m_itemMgr->ConfiscateItem(item->id);
                            m_needsRefresh = true;
                        }
                    }
                    ImGui::PopStyleColor(2);
                    ImGui::SameLine();
                }

                if (ImGui::SmallButton("수여...")) {
                    m_selectedItemId = item->id;
                    m_showAssignModal = true;
                    m_selectedOfficerId = -1;
                    m_assignSearchBuffer[0] = '\0';
                    m_assignAutomata.Clear();
                }

                // 주인공에게 원클릭 수여
                if (protId <= 2164 && item->owner_id != protId) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.45f, 0.15f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.58f, 0.20f, 1.0f));
                    if (ImGui::SmallButton("★주인공 지급")) {
                        if (m_itemMgr) {
                            m_itemMgr->GrantItem(item->id, protId);
                            m_needsRefresh = true;
                        }
                    }
                    ImGui::PopStyleColor(2);
                }

                ImGui::PopID();
            }
        }

        ImGui::EndTable();
    }
}

void ItemTableView::RenderAssignModal() {
    if (!m_showAssignModal) return;

    ImGui::OpenPopup("명품 수여 / 소유자 지정##AssignItemModal");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 520), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("명품 수여 / 소유자 지정##AssignItemModal", &m_showAssignModal, ImGuiWindowFlags_NoResize)) {
        std::string_view itemName = Game::GetItemOfficialName(m_selectedItemId);
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "수여 대상 명품: %s (ID: %u)", itemName.data(), m_selectedItemId);
        ImGui::Separator();

        ImGui::Text("무장 검색:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        HangulInputText("##AssignOfficerSearch", "무장 이름 검색...", m_assignSearchBuffer, sizeof(m_assignSearchBuffer), m_assignAutomata);

        ImGui::SameLine();
        if (ImGui::Button("초기화##ClearAssignSearch")) {
            m_assignSearchBuffer[0] = '\0';
            m_assignAutomata.Clear();
        }

        // 주인공에게 즉시 수여 버튼
        uint16_t protId = m_officerMgr ? m_officerMgr->GetProtagonistOfficerId() : 0xFFFF;
        if (protId <= 2164) {
            std::string_view protName = Game::GetOfficerKoreanName(protId);
            std::string btnLabel = "★ 주인공(" + std::string(protName) + ")에게 즉시 수여";
            if (ImGui::Button(btnLabel.c_str(), ImVec2(-1, 28))) {
                if (m_itemMgr) {
                    m_itemMgr->GrantItem(m_selectedItemId, protId);
                    m_needsRefresh = true;
                }
                m_showAssignModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::Separator();
        }

        // 무장 리스트 박스
        ImGui::Text("수여할 무장 선택:");
        if (ImGui::BeginListBox("##OfficerCandidates", ImVec2(-1, 300))) {
            std::string searchLower = m_assignSearchBuffer;
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

            // 이름이 있는 전체 무장 슬롯 (역사·고대·신무장·PK 추가무장 1151~1263·신무장 1315+, BUG-01 B-2)
            for (uint16_t oid = 0; oid < Game::OfficerMemoryLayout::MAX_TOTAL_SLOTS; oid++) {
                std::string_view oname = Game::GetOfficerKoreanName(oid);
                if (oname.empty()) continue;

                if (!searchLower.empty()) {
                    std::string nameLower = std::string(oname);
                    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                    if (nameLower.find(searchLower) == std::string::npos &&
                        std::to_string(oid).find(searchLower) == std::string::npos) {
                        continue;
                    }
                }

                char label[128];
                snprintf(label, sizeof(label), "%s (#%u)", oname.data(), oid);
                bool isSelected = (m_selectedOfficerId == oid);
                if (ImGui::Selectable(label, isSelected)) {
                    m_selectedOfficerId = oid;
                }
            }
            ImGui::EndListBox();
        }

        ImGui::Separator();
        if (m_selectedOfficerId >= 0) {
            std::string_view selName = Game::GetOfficerKoreanName(m_selectedOfficerId);
            ImGui::Text("선택된 무장: %s (ID: %d)", selName.data(), m_selectedOfficerId);
        } else {
            ImGui::TextDisabled("무장을 선택해 주세요.");
        }

        if (ImGui::Button("수여 확정", ImVec2(120, 30))) {
            if (m_selectedOfficerId >= 0 && m_itemMgr) {
                m_itemMgr->GrantItem(m_selectedItemId, static_cast<uint16_t>(m_selectedOfficerId));
                m_needsRefresh = true;
            }
            m_showAssignModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("취소", ImVec2(120, 30))) {
            m_showAssignModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace San13::UI
