#include "ui/CityTableView.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <chrono>

namespace San13::UI {

CityTableView::CityTableView() = default;
CityTableView::~CityTableView() = default;

void CityTableView::Initialize(Game::CityManager* cityMgr, Game::SettlementManager* setMgr) {
    m_cityMgr = cityMgr;
    m_setMgr = setMgr;
    m_detailModal.Initialize(cityMgr, setMgr);
    RefreshData();
}

void CityTableView::RefreshData() {
    if (!m_cityMgr) return;

    m_cachedCities.clear();
    m_cityMgr->LoadAllCities(m_cachedCities);

    ApplyFilter();
}

void CityTableView::SetSearchQuery(const char* query) {
    if (query) {
        std::strncpy(m_searchQuery, query, sizeof(m_searchQuery) - 1);
        m_searchQuery[sizeof(m_searchQuery) - 1] = '\0';
        m_searchAutomata.SetText(m_searchQuery);
    } else {
        m_searchQuery[0] = '\0';
        m_searchAutomata.Clear();
    }
    ApplyFilter();
}

void CityTableView::SortByColumn(int columnId, bool ascending) {
    ApplySort(columnId, ascending);
}

const Game::CityRecord* CityTableView::GetCityAt(size_t filteredIndex) const {
    if (filteredIndex >= m_filteredIndices.size()) return nullptr;
    size_t actualIdx = m_filteredIndices[filteredIndex];
    if (actualIdx >= m_cachedCities.size()) return nullptr;
    return &m_cachedCities[actualIdx];
}

void CityTableView::ApplyFilter() {
    m_filteredIndices.clear();
    m_filteredIndices.reserve(m_cachedCities.size());

    std::string query = m_searchQuery;
    bool hasQuery = !query.empty();
    std::string queryLower = query;
    if (hasQuery) {
        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
    }

    for (size_t i = 0; i < m_cachedCities.size(); i++) {
        const auto& city = m_cachedCities[i];

        if (hasQuery) {
            std::string idStr = std::to_string(city.id);
            std::string nameLower = city.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            if (nameLower.find(queryLower) == std::string::npos && idStr.find(query) == std::string::npos) {
                continue;
            }
        }

        m_filteredIndices.push_back(i);
    }

    if (m_lastSortColumn >= 0) {
        ApplySort(m_lastSortColumn, m_lastSortAscending);
    }
}

void CityTableView::ApplySort(int columnId, bool ascending) {
    m_lastSortColumn = columnId;
    m_lastSortAscending = ascending;

    std::stable_sort(m_filteredIndices.begin(), m_filteredIndices.end(),
        [this, columnId, ascending](size_t aIdx, size_t bIdx) {
            const auto& a = m_cachedCities[aIdx];
            const auto& b = m_cachedCities[bIdx];

            int cmp = 0;
            switch (columnId) {
                case 0: // ID
                    cmp = (a.id < b.id) ? -1 : (a.id > b.id ? 1 : 0);
                    break;
                case 1: // 도시명
                    cmp = a.name.compare(b.name);
                    break;
                case 2: // 자금
                    cmp = (a.gold < b.gold) ? -1 : (a.gold > b.gold ? 1 : 0);
                    break;
                case 3: // 군량
                    cmp = (a.provisions < b.provisions) ? -1 : (a.provisions > b.provisions ? 1 : 0);
                    break;
                case 4: // 병력
                    cmp = (a.troops < b.troops) ? -1 : (a.troops > b.troops ? 1 : 0);
                    break;
                case 5: // 내구도
                    cmp = (a.durability < b.durability) ? -1 : (a.durability > b.durability ? 1 : 0);
                    break;
                default:
                    break;
            }

            if (cmp != 0) {
                return ascending ? (cmp < 0) : (cmp > 0);
            }
            return a.id < b.id; // 동점 시 ID 오름차순 안정 정렬
        });
}

void CityTableView::OpenEditModal(int cityId) {
    if (!m_cityMgr || cityId < 0 || cityId >= Game::CityMemoryLayout::MAX_CITY_COUNT) return;
    m_selectedCityId = cityId;
    m_detailModal.Open(cityId);
}

bool CityTableView::IsEditModalOpen() const {
    return m_detailModal.IsOpen();
}

void CityTableView::CloseEditModal() {
    m_detailModal.Close();
}

bool CityTableView::CommitEditToMemory() {
    bool ok = m_detailModal.CommitEditToMemory();
    if (ok) {
        RefreshData();
    }
    return ok;
}

bool CityTableView::QuickCheatMaxResources(uint8_t cityId) {
    if (!m_cityMgr || cityId >= Game::CityMemoryLayout::MAX_CITY_COUNT) return false;
    bool ok = m_cityMgr->MaximizeCityResources(cityId);
    if (m_detailModal.IsOpen() && m_detailModal.GetCityId() == cityId) {
        m_detailModal.RefreshData();
    }
    RefreshData();
    return ok;
}

bool CityTableView::QuickCheatDepleteResources(uint8_t cityId) {
    if (!m_cityMgr || cityId >= Game::CityMemoryLayout::MAX_CITY_COUNT) return false;
    m_cityMgr->SetCityGold(cityId, 0, Game::ClampingPolicy::SafeVanilla);
    m_cityMgr->SetCityProvisions(cityId, 0, Game::ClampingPolicy::SafeVanilla);
    m_cityMgr->SetCityTroops(cityId, 0, Game::ClampingPolicy::SafeVanilla);
    if (m_detailModal.IsOpen() && m_detailModal.GetCityId() == cityId) {
        m_detailModal.RefreshData();
    }
    RefreshData();
    return true;
}

bool CityTableView::QuickCheatAllCitiesMax() {
    if (!m_cityMgr) return false;
    for (uint8_t i = 0; i < Game::CityMemoryLayout::MAX_CITY_COUNT; i++) {
        m_cityMgr->MaximizeCityResources(i);
    }
    if (m_detailModal.IsOpen()) {
        m_detailModal.RefreshData();
    }
    RefreshData();
    return true;
}

void CityTableView::SetLimitBreakMode(bool enabled) {
    m_detailModal.SetLimitBreakMode(enabled);
}

bool CityTableView::IsLimitBreakMode() const {
    return m_detailModal.IsLimitBreakMode();
}

void CityTableView::SetAutoSync(bool enabled) {
    m_detailModal.SetAutoSync(enabled);
}

bool CityTableView::IsAutoSync() const {
    return m_detailModal.IsAutoSync();
}

Game::CityRecord& CityTableView::GetEditRecord() {
    return m_detailModal.GetEditingRecord();
}

const Game::CityRecord& CityTableView::GetEditRecord() const {
    return m_detailModal.GetEditingRecord();
}

int CityTableView::GetEditingCityId() const {
    return m_detailModal.GetCityId();
}

float CityTableView::GetLastSyncTimeMs() const {
    return m_detailModal.GetLastSyncTimeMs();
}

static void RenderResourceValue(uint32_t val, uint32_t highThresh) {
    if (val >= highThresh) {
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "%u", val); // 최대치 황금색
    } else if (val == 0) {
        ImGui::TextColored(ImVec4(0.85f, 0.30f, 0.30f, 1.0f), "0 (고갈)"); // 0일 때 붉은색 경고
    } else {
        ImGui::Text("%u", val);
    }
}

void CityTableView::Render() {
    // 1. 상단 컨트롤 바
    ImGui::BeginGroup();

    ImGui::SetNextItemWidth(260.0f);
    if (HangulInputText("##CitySearch", "도시명 또는 ID 검색...", m_searchQuery, sizeof(m_searchQuery), m_searchAutomata)) {
        ApplyFilter();
    }
    if (m_searchQuery[0] != '\0') {
        ImGui::SameLine();
        if (ImGui::SmallButton("×##ClearCitySearch")) {
            m_searchQuery[0] = '\0';
            m_searchAutomata.Clear();
            ApplyFilter();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("검색어 초기화 (전체 목록 보기)");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("새로고침 (F5)")) {
        RefreshData();
    }

    if (m_selectedCityId >= 0) {
        ImGui::SameLine();
        if (ImGui::Button("선택 도시 편집")) {
            OpenEditModal(m_selectedCityId);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("전 도시 자원 최대화 (All Max) ★")) {
        QuickCheatAllCitiesMax();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("| 총 %zu개 도시 중 %zu개 표시", m_cachedCities.size(), m_filteredIndices.size());

    ImGui::EndGroup();
    ImGui::Spacing();

    // 2. 도시 목록 테이블
    static ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_Sortable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersV;

    if (ImGui::BeginTable("CityVirtualTable", 6, flags, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("ID",     ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 55.0f, 0);
        ImGui::TableSetupColumn("도시명", ImGuiTableColumnFlags_WidthStretch, 2.0f, 1);
        ImGui::TableSetupColumn("자금",   ImGuiTableColumnFlags_WidthStretch, 1.4f, 2);
        ImGui::TableSetupColumn("군량",   ImGuiTableColumnFlags_WidthStretch, 1.4f, 3);
        ImGui::TableSetupColumn("병력",   ImGuiTableColumnFlags_WidthStretch, 1.3f, 4);
        ImGui::TableSetupColumn("내구도", ImGuiTableColumnFlags_WidthStretch, 1.3f, 5);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        // 정렬 처리
        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty && sortSpecs->SpecsCount > 0) {
                ApplySort(sortSpecs->Specs[0].ColumnIndex,
                          sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
                sortSpecs->SpecsDirty = false;
            }
        }

        for (size_t row = 0; row < m_filteredIndices.size(); row++) {
            size_t actualIdx = m_filteredIndices[row];
            const auto& city = m_cachedCities[actualIdx];

            ImGui::TableNextRow();

            // Column 0: ID
            ImGui::TableSetColumnIndex(0);
            char label[32];
            std::snprintf(label, sizeof(label), "%02d##city_%d", city.id, city.id);
            bool isSelected = (m_selectedCityId == static_cast<int>(city.id));
            if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                m_selectedCityId = city.id;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                OpenEditModal(city.id);
            }
            if (ImGui::BeginPopupContextItem()) {
                m_selectedCityId = city.id;
                ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "■ %s (ID %02d)", city.name.c_str(), city.id);
                ImGui::Separator();
                if (ImGui::MenuItem("상세 자원 편집...")) {
                    OpenEditModal(city.id);
                }
                if (ImGui::MenuItem("자금/군량 999,999 풀충전")) {
                    QuickCheatMaxResources(city.id);
                }
                if (ImGui::MenuItem("성벽 내구도 5,000 완충")) {
                    m_cityMgr->SetCityDurability(city.id, 5000);
                    m_cityMgr->LoadCity(city.id, m_cachedCities[actualIdx]);
                }
                if (ImGui::MenuItem("자원 고갈 치트 (자금/군량 0)")) {
                    QuickCheatDepleteResources(city.id);
                }
                ImGui::EndPopup();
            }

            // Column 1: 도시명
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", city.name.c_str());

            // Column 2: 자금
            ImGui::TableSetColumnIndex(2);
            RenderResourceValue(city.gold, 900000);

            // Column 3: 군량
            ImGui::TableSetColumnIndex(3);
            RenderResourceValue(city.provisions, 900000);

            // Column 4: 병력
            ImGui::TableSetColumnIndex(4);
            RenderResourceValue(city.troops, 100000);

            // Column 5: 내구도
            ImGui::TableSetColumnIndex(5);
            RenderResourceValue(city.durability, 4500);
        }

        ImGui::EndTable();
    }

    RenderEditModal();
}

void CityTableView::RenderEditModal() {
    m_detailModal.Render();
}

} // namespace San13::UI
