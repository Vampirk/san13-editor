#include "ui/OfficerTableView.hpp"
#include "game/OfficerNameTable.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <chrono>

namespace San13::UI {

using namespace San13::Game;

OfficerTableView::OfficerTableView() = default;
OfficerTableView::~OfficerTableView() = default;

void OfficerTableView::Initialize(Game::OfficerManager* officerMgr) {
    m_officerMgr = officerMgr;
    RefreshData();
}

void OfficerTableView::RefreshData() {
    if (!m_officerMgr) return;

    // [EDGE-25] 실시간 시나리오 일자 읽기
    m_officerMgr->GetGameDate(m_gameYear, m_gameMonth, m_gameDay);

    m_cachedOfficers.clear();
    // 800명 역사 무장 + 활성화된 결혼 자녀 및 신무장 슬롯 일괄 로드
    m_officerMgr->LoadAllOfficers(m_cachedOfficers, true);

    ApplyFilter();
}

void OfficerTableView::ApplyFilter() {
    m_filteredIndices.clear();
    m_filteredIndices.reserve(m_cachedOfficers.size());

    std::string query = m_searchQuery;
    bool hasQuery = !query.empty();
    std::string queryLower = query;
    if (hasQuery) {
        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
    }

    uint16_t curYr = m_gameYear;

    for (size_t i = 0; i < m_cachedOfficers.size(); i++) {
        const auto& off = m_cachedOfficers[i];

        // 1. 상태 필터 검사 (EDGE-26/27/29/31)
        uint16_t yr = off.current_year > 0 ? off.current_year : curYr;
        bool isUnappeared = (yr > 0 && off.appear_year > yr);
        bool isFree = !isUnappeared && (off.status_flag == 0 || off.force_id == 0 || off.force_id == 0xFFFF || off.force_id >= 120);
        bool isActive = !isUnappeared && (off.status_flag == 1 && off.force_id > 0 && off.force_id < 120 && off.force_id != 0xFFFF);

        if (m_filterMode == OfficerFilterMode::Active) {
            if (!isActive || off.status_flag == 0xFF) continue;
        } else if (m_filterMode == OfficerFilterMode::Free) {
            if (!isFree || off.status_flag == 0xFF) continue;
        } else if (m_filterMode == OfficerFilterMode::Unappeared) {
            if (!isUnappeared || off.status_flag == 0xFF) continue;
        } else if (m_filterMode == OfficerFilterMode::Extended) {
            if (!off.is_extended) continue;
        }

        // 2. 검색어 필터 검사 (이름 또는 ID, 대소문자 무시, EDGE-31 항적/항우 별칭 지원)
        if (hasQuery) {
            std::string idStr = std::to_string(off.id);
            std::string nameLower = off.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            bool matched = (nameLower.find(queryLower) != std::string::npos || idStr.find(query) != std::string::npos);
            // EDGE-31: 항우로 검색해도 항적이 매칭되도록 별칭 지원
            if (!matched && queryLower == "항우" && nameLower.find("항적") != std::string::npos) {
                matched = true;
            }
            if (!matched) {
                continue;
            }
        }

        m_filteredIndices.push_back(i);
    }

    if (m_lastSortColumn >= 0) {
        ApplySort(m_lastSortColumn, m_lastSortAscending);
    }
}

void OfficerTableView::SetSearchQuery(const char* query) {
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

void OfficerTableView::SetFilterMode(OfficerFilterMode mode) {
    m_filterMode = mode;
    ApplyFilter();
}

void OfficerTableView::SortByColumn(int columnId, bool ascending) {
    ApplySort(columnId, ascending);
}

const Game::OfficerRecord* OfficerTableView::GetOfficerAt(size_t filteredIndex) const {
    if (filteredIndex >= m_filteredIndices.size()) return nullptr;
    size_t actualIdx = m_filteredIndices[filteredIndex];
    if (actualIdx >= m_cachedOfficers.size()) return nullptr;
    return &m_cachedOfficers[actualIdx];
}

void OfficerTableView::OpenEditModal(int officerId) {
    if (!m_officerMgr || officerId < 0) return;
    if (m_officerMgr->LoadOfficer(static_cast<uint16_t>(officerId), m_editingRecord)) {
        m_editingOfficerId = officerId;
        m_selectedOfficerId = officerId;
        m_editStats = m_editingRecord.stats;
        m_showEditModal = true;
        m_syncStatusMsg.clear();
    }
}

bool OfficerTableView::CommitEditToMemory() {
    if (!m_officerMgr || m_editingOfficerId < 0) return false;

    auto policy = m_limitBreakMode ? Game::ClampingPolicy::LimitBreak : Game::ClampingPolicy::SafeVanilla;
    uint16_t id = static_cast<uint16_t>(m_editingOfficerId);

    auto startTime = std::chrono::high_resolution_clock::now();
    bool ok = true;
    ok &= m_officerMgr->WriteStats(id, m_editStats, policy);
    ok &= m_officerMgr->SetTactic(id, m_editingRecord.tactic_id);
    ok &= m_officerMgr->SetSkills(id, m_editingRecord.skills);
    ok &= m_officerMgr->SetFaceId(id, m_editingRecord.face_id);
    ok &= m_officerMgr->SetPersonality(id, m_editingRecord.personality);
    ok &= m_officerMgr->SetCharacter(id, m_editingRecord.character);
    ok &= m_officerMgr->SetRelations(id, m_editingRecord.father_id, m_editingRecord.mother_id, m_editingRecord.spouse_1_id, m_editingRecord.sworn_brother_id,
                                     m_editingRecord.spouse_2_id, m_editingRecord.spouse_3_id, m_editingRecord.rival_id);
    auto endTime = std::chrono::high_resolution_clock::now();
    m_lastSyncTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (ok) {
        // 게임 메모리에서 최신화된 데이터 재로드
        m_officerMgr->LoadOfficer(id, m_editingRecord);
        m_editStats = m_editingRecord.stats;

        // 캐시 목록 즉시 업데이트
        for (auto& off : m_cachedOfficers) {
            if (off.id == static_cast<uint32_t>(m_editingOfficerId)) {
                off = m_editingRecord;
                break;
            }
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "● 실시간 핫싱크 성공 (%.3fms)", m_lastSyncTimeMs);
        m_syncStatusMsg = buf;
    } else {
        m_syncStatusMsg = "× 메모리 쓰기 실패";
    }

    return ok;
}

bool OfficerTableView::QuickCheatAll100(uint16_t officerId) {
    if (!m_officerMgr) return false;
    Game::OfficerRecord rec;
    if (!m_officerMgr->LoadOfficer(officerId, rec)) return false;

    rec.stats.base_leadership = 100; rec.stats.curr_leadership = 100;
    rec.stats.base_war = 100;        rec.stats.curr_war = 100;
    rec.stats.base_intel = 100;      rec.stats.curr_intel = 100;
    rec.stats.base_politics = 100;   rec.stats.curr_politics = 100;

    bool ok = m_officerMgr->WriteStats(officerId, rec.stats, Game::ClampingPolicy::SafeVanilla);
    if (ok) {
        for (auto& off : m_cachedOfficers) {
            if (off.id == officerId) {
                m_officerMgr->LoadOfficer(off.id, off);
                if (m_editingOfficerId == static_cast<int>(officerId)) {
                    m_editingRecord = off;
                    m_editStats = off.stats;
                }
                break;
            }
        }
    }
    return ok;
}

bool OfficerTableView::QuickCheatAll255(uint16_t officerId) {
    if (!m_officerMgr) return false;
    Game::OfficerRecord rec;
    if (!m_officerMgr->LoadOfficer(officerId, rec)) return false;

    rec.stats.base_leadership = 255; rec.stats.curr_leadership = 255;
    rec.stats.base_war = 255;        rec.stats.curr_war = 255;
    rec.stats.base_intel = 255;      rec.stats.curr_intel = 255;
    rec.stats.base_politics = 255;   rec.stats.curr_politics = 255;

    bool ok = m_officerMgr->WriteStats(officerId, rec.stats, Game::ClampingPolicy::LimitBreak);
    if (ok) {
        for (auto& off : m_cachedOfficers) {
            if (off.id == officerId) {
                m_officerMgr->LoadOfficer(off.id, off);
                if (m_editingOfficerId == static_cast<int>(officerId)) {
                    m_editingRecord = off;
                    m_editStats = off.stats;
                }
                break;
            }
        }
    }
    return ok;
}

bool OfficerTableView::QuickCheatAllAptitudeS(uint16_t officerId) {
    if (!m_officerMgr) return false;
    Game::OfficerRecord rec;
    if (!m_officerMgr->LoadOfficer(officerId, rec)) return false;

    rec.stats.spear_apt = 3;
    rec.stats.horse_apt = 3;
    rec.stats.bow_apt   = 3;

    bool ok = m_officerMgr->WriteStats(officerId, rec.stats, Game::ClampingPolicy::SafeVanilla);
    if (ok) {
        for (auto& off : m_cachedOfficers) {
            if (off.id == officerId) {
                m_officerMgr->LoadOfficer(off.id, off);
                if (m_editingOfficerId == static_cast<int>(officerId)) {
                    m_editingRecord = off;
                    m_editStats = off.stats;
                }
                break;
            }
        }
    }
    return ok;
}

// --- Phase 5.5 다중 선택 및 일괄 수정 로직 ---

bool OfficerTableView::IsOfficerSelected(uint16_t id) const {
    return m_selectedOfficerIds.find(id) != m_selectedOfficerIds.end();
}

void OfficerTableView::SelectOfficer(uint16_t id, bool selected) {
    if (selected) {
        m_selectedOfficerIds.insert(id);
    } else {
        m_selectedOfficerIds.erase(id);
    }
}

void OfficerTableView::ToggleSelectOfficer(uint16_t id) {
    auto it = m_selectedOfficerIds.find(id);
    if (it != m_selectedOfficerIds.end()) {
        m_selectedOfficerIds.erase(it);
    } else {
        m_selectedOfficerIds.insert(id);
    }
}

void OfficerTableView::SelectAllFiltered() {
    for (size_t idx : m_filteredIndices) {
        if (idx < m_cachedOfficers.size()) {
            m_selectedOfficerIds.insert(m_cachedOfficers[idx].id);
        }
    }
}

void OfficerTableView::ClearSelection() {
    m_selectedOfficerIds.clear();
}

void OfficerTableView::InvertSelection() {
    std::unordered_set<uint16_t> inverted;
    for (size_t idx : m_filteredIndices) {
        if (idx < m_cachedOfficers.size()) {
            uint16_t id = m_cachedOfficers[idx].id;
            if (m_selectedOfficerIds.find(id) == m_selectedOfficerIds.end()) {
                inverted.insert(id);
            }
        }
    }
    m_selectedOfficerIds = std::move(inverted);
}

void OfficerTableView::SelectSameForceAs(uint16_t officerId) {
    uint16_t forceId = 0xFFFF;
    for (const auto& off : m_cachedOfficers) {
        if (off.id == officerId) {
            forceId = off.force_id;
            break;
        }
    }
    if (forceId == 0xFFFF) return;
    SelectByForce(forceId);
}

void OfficerTableView::SelectByForce(uint16_t forceId) {
    m_selectedOfficerIds.clear();
    for (const auto& off : m_cachedOfficers) {
        if (off.force_id == forceId && off.status_flag != 0xFF) {
            m_selectedOfficerIds.insert(off.id);
        }
    }
}

void OfficerTableView::SelectSameCityAs(uint16_t officerId) {
    uint16_t cityId = 0xFFFF;
    for (const auto& off : m_cachedOfficers) {
        if (off.id == officerId) {
            cityId = off.city_id;
            break;
        }
    }
    if (cityId == 0xFFFF || cityId == 255) return;
    SelectByCity(cityId);
}

void OfficerTableView::SelectByCity(uint16_t cityId) {
    m_selectedOfficerIds.clear();
    for (const auto& off : m_cachedOfficers) {
        if (off.city_id == cityId && off.status_flag != 0xFF) {
            m_selectedOfficerIds.insert(off.id);
        }
    }
}

void OfficerTableView::SelectByRank(uint8_t rank) {
    m_selectedOfficerIds.clear();
    for (const auto& off : m_cachedOfficers) {
        if (off.rank == rank && off.status_flag != 0xFF) {
            m_selectedOfficerIds.insert(off.id);
        }
    }
}

void OfficerTableView::OpenBatchModal() {
    if (m_selectedOfficerIds.empty()) return;
    m_showBatchModal = true;
    m_batchStatusMsg.clear();
}

bool OfficerTableView::ApplyBatchStatsDelta(int dLead, int dWar, int dIntel, int dPol) {
    if (!m_officerMgr || m_selectedOfficerIds.empty()) return false;

    std::vector<uint16_t> ids(m_selectedOfficerIds.begin(), m_selectedOfficerIds.end());
    m_officerMgr->CreateSnapshots(ids, m_lastBatchSnapshots);

    auto policy = m_limitBreakMode ? Game::ClampingPolicy::LimitBreak : Game::ClampingPolicy::SafeVanilla;

    auto startTime = std::chrono::high_resolution_clock::now();
    bool ok = m_officerMgr->BatchApplyStatsDelta(ids, dLead, dWar, dIntel, dPol, policy);
    auto endTime = std::chrono::high_resolution_clock::now();
    m_lastBatchTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (ok) {
        RefreshData();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "● %zu명 스탯 일괄 가산 완료 (%.3fms)", ids.size(), m_lastBatchTimeMs);
        m_batchStatusMsg = buf;
    } else {
        m_batchStatusMsg = "× 일부 무장 메모리 쓰기 실패";
    }
    return ok;
}

bool OfficerTableView::ApplyBatchStatsSet(int lead, int war, int intel, int pol,
                                         bool setL, bool setW, bool setI, bool setP) {
    if (!m_officerMgr || m_selectedOfficerIds.empty()) return false;

    std::vector<uint16_t> ids(m_selectedOfficerIds.begin(), m_selectedOfficerIds.end());
    m_officerMgr->CreateSnapshots(ids, m_lastBatchSnapshots);

    auto policy = m_limitBreakMode ? Game::ClampingPolicy::LimitBreak : Game::ClampingPolicy::SafeVanilla;

    auto startTime = std::chrono::high_resolution_clock::now();
    bool ok = m_officerMgr->BatchApplyStatsSet(ids, lead, war, intel, pol, setL, setW, setI, setP, policy);
    auto endTime = std::chrono::high_resolution_clock::now();
    m_lastBatchTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (ok) {
        RefreshData();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "● %zu명 스탯 일괄 설정 완료 (%.3fms)", ids.size(), m_lastBatchTimeMs);
        m_batchStatusMsg = buf;
    } else {
        m_batchStatusMsg = "× 일부 무장 메모리 쓰기 실패";
    }
    return ok;
}

bool OfficerTableView::ApplyBatchAptitudes(int spear, int horse, int bow) {
    if (!m_officerMgr || m_selectedOfficerIds.empty()) return false;

    std::vector<uint16_t> ids(m_selectedOfficerIds.begin(), m_selectedOfficerIds.end());
    m_officerMgr->CreateSnapshots(ids, m_lastBatchSnapshots);

    auto startTime = std::chrono::high_resolution_clock::now();
    bool ok = m_officerMgr->BatchApplyAptitudes(ids, spear, horse, bow);
    auto endTime = std::chrono::high_resolution_clock::now();
    m_lastBatchTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (ok) {
        RefreshData();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "● %zu명 병종 적성 일괄 승급 완료 (%.3fms)", ids.size(), m_lastBatchTimeMs);
        m_batchStatusMsg = buf;
    } else {
        m_batchStatusMsg = "× 일부 무장 적성 쓰기 실패";
    }
    return ok;
}

bool OfficerTableView::ApplyBatchLifeExtension(int addYears) {
    if (!m_officerMgr || m_selectedOfficerIds.empty()) return false;

    std::vector<uint16_t> ids(m_selectedOfficerIds.begin(), m_selectedOfficerIds.end());
    m_officerMgr->CreateSnapshots(ids, m_lastBatchSnapshots);

    auto startTime = std::chrono::high_resolution_clock::now();
    bool ok = m_officerMgr->BatchExtendLife(ids, addYears);
    auto endTime = std::chrono::high_resolution_clock::now();
    m_lastBatchTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (ok) {
        RefreshData();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "● %zu명 수명 +%d년 연장 완료 (%.3fms)", ids.size(), addYears, m_lastBatchTimeMs);
        m_batchStatusMsg = buf;
    } else {
        m_batchStatusMsg = "× 일부 무장 수명 연장 실패";
    }
    return ok;
}

bool OfficerTableView::BatchCheatAllBuff10() {
    return ApplyBatchStatsDelta(10, 10, 10, 10);
}

bool OfficerTableView::BatchCheatAll100() {
    return ApplyBatchStatsSet(100, 100, 100, 100, true, true, true, true);
}

bool OfficerTableView::BatchCheatAll255() {
    m_limitBreakMode = true;
    return ApplyBatchStatsSet(255, 255, 255, 255, true, true, true, true);
}

bool OfficerTableView::BatchCheatAllAptitudeS() {
    return ApplyBatchAptitudes(3, 3, 3);
}

bool OfficerTableView::BatchCheatLifeExt30() {
    return ApplyBatchLifeExtension(30);
}

bool OfficerTableView::UndoLastBatchOperation() {
    if (!m_officerMgr || m_lastBatchSnapshots.empty()) return false;

    auto startTime = std::chrono::high_resolution_clock::now();
    bool ok = m_officerMgr->BatchRestore(m_lastBatchSnapshots);
    auto endTime = std::chrono::high_resolution_clock::now();
    float undoTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (ok) {
        RefreshData();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "● %zu명 일괄 수정 롤백(Undo) 성공 (%.3fms)", m_lastBatchSnapshots.size(), undoTimeMs);
        m_batchStatusMsg = buf;
        m_lastBatchSnapshots.clear();
    } else {
        m_batchStatusMsg = "× 롤백 복구 실패";
    }
    return ok;
}

void OfficerTableView::ApplySort(int columnId, bool ascending) {
    m_lastSortColumn = columnId;
    m_lastSortAscending = ascending;

    std::stable_sort(m_filteredIndices.begin(), m_filteredIndices.end(),
        [this, columnId, ascending](size_t aIdx, size_t bIdx) {
            const auto& a = m_cachedOfficers[aIdx];
            const auto& b = m_cachedOfficers[bIdx];

            int cmp = 0;
            switch (columnId) {
                case 0:  // ID
                    cmp = (a.id < b.id) ? -1 : (a.id > b.id ? 1 : 0);
                    break;
                case 1:  // 이름
                    cmp = a.name.compare(b.name);
                    break;
                case 2:  // 신분/상태
                    cmp = (a.rank < b.rank) ? -1 : (a.rank > b.rank ? 1 : 0);
                    break;
                case 3:  // 통솔
                    cmp = (a.stats.curr_leadership < b.stats.curr_leadership) ? -1 : (a.stats.curr_leadership > b.stats.curr_leadership ? 1 : 0);
                    break;
                case 4:  // 무력
                    cmp = (a.stats.curr_war < b.stats.curr_war) ? -1 : (a.stats.curr_war > b.stats.curr_war ? 1 : 0);
                    break;
                case 5:  // 지력
                    cmp = (a.stats.curr_intel < b.stats.curr_intel) ? -1 : (a.stats.curr_intel > b.stats.curr_intel ? 1 : 0);
                    break;
                case 6:  // 정치
                    cmp = (a.stats.curr_politics < b.stats.curr_politics) ? -1 : (a.stats.curr_politics > b.stats.curr_politics ? 1 : 0);
                    break;
                case 7:  // 총합
                    cmp = (a.GetTotalStats() < b.GetTotalStats()) ? -1 : (a.GetTotalStats() > b.GetTotalStats() ? 1 : 0);
                    break;
                case 8:  // 창병
                    cmp = (a.stats.spear_apt < b.stats.spear_apt) ? -1 : (a.stats.spear_apt > b.stats.spear_apt ? 1 : 0);
                    break;
                case 9:  // 기병
                    cmp = (a.stats.horse_apt < b.stats.horse_apt) ? -1 : (a.stats.horse_apt > b.stats.horse_apt ? 1 : 0);
                    break;
                case 10: // 궁병
                    cmp = (a.stats.bow_apt < b.stats.bow_apt) ? -1 : (a.stats.bow_apt > b.stats.bow_apt ? 1 : 0);
                    break;
                case 11: // 출생
                    cmp = (a.birth_year < b.birth_year) ? -1 : (a.birth_year > b.birth_year ? 1 : 0);
                    break;
                case 12: // 등장
                    cmp = (a.appear_year < b.appear_year) ? -1 : (a.appear_year > b.appear_year ? 1 : 0);
                    break;
                case 13: // 사망
                    cmp = (a.death_year < b.death_year) ? -1 : (a.death_year > b.death_year ? 1 : 0);
                    break;
                default:
                    break;
            }

            if (cmp != 0) {
                return ascending ? (cmp < 0) : (cmp > 0);
            }
            return a.id < b.id; // 동점 시 ID 오름차순으로 안정적 정렬 보장
        });
}

static void RenderAptitudeText(uint8_t apt) {
    const char* text = Game::OfficerRecord::AptitudeToString(apt);
    ImVec4 color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // C: 회색
    if ((apt & 0x03) == 3)      color = ImVec4(0.95f, 0.82f, 0.40f, 1.0f); // S: 황금색
    else if ((apt & 0x03) == 2) color = ImVec4(0.30f, 0.85f, 0.95f, 1.0f); // A: 시안
    else if ((apt & 0x03) == 1) color = ImVec4(0.35f, 0.90f, 0.45f, 1.0f); // B: 연녹색

    ImGui::TextColored(color, "%s", text);
}

static void RenderStatValue(uint8_t val) {
    if (val >= 90) {
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "%d", val); // 90 이상 황금색
    } else if (val >= 80) {
        ImGui::TextColored(ImVec4(0.40f, 0.90f, 0.50f, 1.0f), "%d", val); // 80 이상 연녹색
    } else if (val <= 40) {
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.60f, 1.0f), "%d", val); // 40 이하 딤그레이
    } else {
        ImGui::Text("%d", val);
    }
}

void OfficerTableView::Render() {
    // 1. 상단 컨트롤 바 (검색창 + 필터 탭 + 새로고침 + 다중 선택 도구)
    ImGui::BeginGroup();

    ImGui::SetNextItemWidth(260.0f);
    if (HangulInputText("##OfficerSearch", "무장 성명 또는 ID 검색...", m_searchQuery, sizeof(m_searchQuery), m_searchAutomata)) {
        ApplyFilter();
    }
    if (m_searchQuery[0] != '\0') {
        ImGui::SameLine();
        if (ImGui::SmallButton("×##ClearOfficerSearch")) {
            m_searchQuery[0] = '\0';
            m_searchAutomata.Clear();
            ApplyFilter();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("검색어 초기화 (전체 목록 보기)");
        }
    }

    ImGui::SameLine();
    int currentFilter = static_cast<int>(m_filterMode);
    if (ImGui::RadioButton("전체", &currentFilter, 0)) { m_filterMode = OfficerFilterMode::All; ApplyFilter(); }
    ImGui::SameLine();
    if (ImGui::RadioButton("현역", &currentFilter, 1)) { m_filterMode = OfficerFilterMode::Active; ApplyFilter(); }
    ImGui::SameLine();
    if (ImGui::RadioButton("재야", &currentFilter, 2)) { m_filterMode = OfficerFilterMode::Free; ApplyFilter(); }
    ImGui::SameLine();
    if (ImGui::RadioButton("미등장", &currentFilter, 3)) { m_filterMode = OfficerFilterMode::Unappeared; ApplyFilter(); }
    ImGui::SameLine();
    if (ImGui::RadioButton("신무장/자녀 (800+)", &currentFilter, 4)) { m_filterMode = OfficerFilterMode::Extended; ApplyFilter(); }

    ImGui::SameLine();
    if (ImGui::Button("새로고침 (F5)")) {
        RefreshData();
    }

    // [EDGE-25] 실시간 게임 일자 뱃지
    if (m_gameYear > 0) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "📅 게임 일자: %d년 %d월 %d일", m_gameYear, m_gameMonth, m_gameDay);
    }

    // [EDGE-30] 2단 분리 레이아웃: 다중 선택 및 액션 도구를 2번째 줄로 줄바꿈하여 버튼 잘림 100% 해소
    ImGui::Spacing();

    if (ImGui::Button("전체 선택")) {
        SelectAllFiltered();
    }
    ImGui::SameLine();
    if (ImGui::Button("선택 해제")) {
        ClearSelection();
    }
    ImGui::SameLine();
    if (ImGui::Button("반전")) {
        InvertSelection();
    }
    if (m_selectedOfficerId >= 0) {
        ImGui::SameLine();
        if (ImGui::Button("아군 선택")) {
            SelectSameForceAs(static_cast<uint16_t>(m_selectedOfficerId));
        }
    }

    if (m_selectedOfficerId >= 0) {
        ImGui::SameLine();
        if (ImGui::Button("선택 무장 편집")) {
            OpenEditModal(m_selectedOfficerId);
        }
    }

    if (!m_selectedOfficerIds.empty()) {
        ImGui::SameLine();
        char batchBtn[64];
        std::snprintf(batchBtn, sizeof(batchBtn), "일괄 수정 (%zu명) ★", m_selectedOfficerIds.size());
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.45f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.55f, 0.20f, 1.0f));
        if (ImGui::Button(batchBtn)) {
            OpenBatchModal();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();
    ImGui::TextDisabled("| 총 %zu명 중 %zu명 표시 (선택: %zu명)", m_cachedOfficers.size(), m_filteredIndices.size(), m_selectedOfficerIds.size());

    ImGui::EndGroup();
    ImGui::Spacing();

    // 2. 가상 스크롤 ImGui 테이블
    static ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_Sortable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersV;

    if (ImGui::BeginTable("OfficerVirtualTable", 14, flags, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("선택/ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 80.0f, 0);
        ImGui::TableSetupColumn("성명",    ImGuiTableColumnFlags_WidthFixed, 140.0f, 1);
        ImGui::TableSetupColumn("신분",    ImGuiTableColumnFlags_WidthFixed, 90.0f, 2);
        ImGui::TableSetupColumn("통솔",    ImGuiTableColumnFlags_WidthFixed, 55.0f, 3);
        ImGui::TableSetupColumn("무력",    ImGuiTableColumnFlags_WidthFixed, 55.0f, 4);
        ImGui::TableSetupColumn("지력",    ImGuiTableColumnFlags_WidthFixed, 55.0f, 5);
        ImGui::TableSetupColumn("정치",    ImGuiTableColumnFlags_WidthFixed, 55.0f, 6);
        ImGui::TableSetupColumn("총합",    ImGuiTableColumnFlags_WidthFixed, 60.0f, 7);
        ImGui::TableSetupColumn("창병",    ImGuiTableColumnFlags_WidthFixed, 45.0f, 8);
        ImGui::TableSetupColumn("기병",    ImGuiTableColumnFlags_WidthFixed, 45.0f, 9);
        ImGui::TableSetupColumn("궁병",    ImGuiTableColumnFlags_WidthFixed, 45.0f, 10);
        ImGui::TableSetupColumn("출생",    ImGuiTableColumnFlags_WidthFixed, 55.0f, 11);
        ImGui::TableSetupColumn("등장",    ImGuiTableColumnFlags_WidthFixed, 55.0f, 12);
        ImGui::TableSetupColumn("사망",    ImGuiTableColumnFlags_WidthFixed, 55.0f, 13);
        ImGui::TableSetupScrollFreeze(0, 1); // 첫 번째 헤더 행 상단 고정
        ImGui::TableHeadersRow();

        // 정렬 처리
        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty && sortSpecs->SpecsCount > 0) {
                ApplySort(sortSpecs->Specs[0].ColumnIndex,
                          sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
                sortSpecs->SpecsDirty = false;
            }
        }

        // 고속 가상 스크롤러 (화면에 보이는 25~35행만 렌더링)
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_filteredIndices.size()));

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                size_t actualIdx = m_filteredIndices[row];
                const auto& off = m_cachedOfficers[actualIdx];

                ImGui::TableNextRow();

                // Column 0: 선택 체크박스 + ID
                ImGui::TableSetColumnIndex(0);
                bool isRowChecked = IsOfficerSelected(off.id);
                char chkId[32];
                std::snprintf(chkId, sizeof(chkId), "##chk_%d", off.id);
                if (ImGui::Checkbox(chkId, &isRowChecked)) {
                    SelectOfficer(off.id, isRowChecked);
                }
                ImGui::SameLine();
                char label[32];
                std::snprintf(label, sizeof(label), "%03d##off_%d", off.id, off.id);
                bool isSelected = (m_selectedOfficerId == static_cast<int>(off.id));
                if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                    ImGuiIO& io = ImGui::GetIO();
                    if (io.KeyCtrl) {
                        ToggleSelectOfficer(off.id);
                    } else if (io.KeyShift && m_lastClickedRow >= 0) {
                        int rStart = std::min(m_lastClickedRow, row);
                        int rEnd = std::max(m_lastClickedRow, row);
                        for (int r = rStart; r <= rEnd; r++) {
                            if (r >= 0 && r < static_cast<int>(m_filteredIndices.size())) {
                                size_t oIdx = m_filteredIndices[r];
                                if (oIdx < m_cachedOfficers.size()) {
                                    m_selectedOfficerIds.insert(m_cachedOfficers[oIdx].id);
                                }
                            }
                        }
                    } else {
                        m_selectedOfficerId = off.id;
                    }
                    m_lastClickedRow = row;
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    OpenEditModal(off.id);
                }
                if (ImGui::BeginPopupContextItem()) {
                    m_selectedOfficerId = off.id;
                    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "■ %s (ID %03d)", off.name.c_str(), off.id);
                    ImGui::Separator();
                    if (ImGui::MenuItem(isRowChecked ? "이 무장 선택 해제" : "이 무장 선택")) {
                        ToggleSelectOfficer(off.id);
                    }
                    if (ImGui::MenuItem("이 무장 소속 세력(아군) 전체 선택")) {
                        SelectSameForceAs(off.id);
                    }
                    if (!m_selectedOfficerIds.empty()) {
                        char batchItem[64];
                        std::snprintf(batchItem, sizeof(batchItem), "선택된 %zu명 일괄 수정 모달...", m_selectedOfficerIds.size());
                        if (ImGui::MenuItem(batchItem)) {
                            OpenBatchModal();
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("상세 능력치 편집...")) {
                        OpenEditModal(off.id);
                    }
                    if (ImGui::MenuItem("능력치 올 100 치트 (바닐라)")) {
                        QuickCheatAll100(off.id);
                    }
                    if (ImGui::MenuItem("전 병종 S급 적성 치트")) {
                        QuickCheatAllAptitudeS(off.id);
                    }
                    if (ImGui::MenuItem("한계 돌파 올 255 치트 (패왕)")) {
                        QuickCheatAll255(off.id);
                    }
                    ImGui::EndPopup();
                }

                // Column 1: 성명
                ImGui::TableSetColumnIndex(1);
                if (off.is_extended) {
                    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "★ %s", off.name.c_str());
                } else {
                    ImGui::Text("%s", off.name.c_str());
                }

                // Column 2: 신분
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", off.GetStatusDescription(m_gameYear));

                // Column 3~6: 4대 스탯
                ImGui::TableSetColumnIndex(3); RenderStatValue(off.stats.curr_leadership);
                ImGui::TableSetColumnIndex(4); RenderStatValue(off.stats.curr_war);
                ImGui::TableSetColumnIndex(5); RenderStatValue(off.stats.curr_intel);
                ImGui::TableSetColumnIndex(6); RenderStatValue(off.stats.curr_politics);

                // Column 7: 총합
                ImGui::TableSetColumnIndex(7);
                int total = off.GetTotalStats();
                if (total >= 350) {
                    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "%d", total);
                } else {
                    ImGui::Text("%d", total);
                }

                // Column 8~10: 병종 적성 (C/B/A/S)
                ImGui::TableSetColumnIndex(8);  RenderAptitudeText(off.stats.spear_apt);
                ImGui::TableSetColumnIndex(9);  RenderAptitudeText(off.stats.horse_apt);
                ImGui::TableSetColumnIndex(10); RenderAptitudeText(off.stats.bow_apt);

                // Column 11~13: 생몰년도
                ImGui::TableSetColumnIndex(11);
                if (off.birth_year > 0) ImGui::Text("%d", off.birth_year); else ImGui::TextDisabled("-");

                ImGui::TableSetColumnIndex(12);
                if (off.appear_year > 0) ImGui::Text("%d", off.appear_year); else ImGui::TextDisabled("-");

                ImGui::TableSetColumnIndex(13);
                if (off.death_year > 0) ImGui::Text("%d", off.death_year); else ImGui::TextDisabled("-");
            }
        }

        clipper.End();
        ImGui::EndTable();
    }

    RenderEditModal();
    RenderBatchEditModal();
}

void OfficerTableView::RenderEditModal() {
    static const char* modalTitle = "무장 능력치 및 적성 상세 편집##OfficerEditModal";
    if (m_showEditModal && !ImGui::IsPopupOpen(modalTitle)) {
        ImGui::OpenPopup(modalTitle);
    }

    ImGui::SetNextWindowSize(ImVec2(680, 580), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal(modalTitle, &m_showEditModal, ImGuiWindowFlags_None)) {
        // 1. 헤더 정보
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "■ %s (ID %03d)",
                           m_editingRecord.name.c_str(), m_editingRecord.id);
        ImGui::SameLine();
        ImGui::TextDisabled("| 소속: %s | 신분: %s | 소재: 도시 %d",
                            m_editingRecord.force_id == 0xFFFF ? "재야" : ("세력 " + std::to_string(m_editingRecord.force_id)).c_str(),
                            m_editingRecord.GetStatusDescription(m_gameYear),
                            m_editingRecord.city_id);

        ImGui::Separator();

        // 제어 옵션 (한계 돌파 모드 & 자동 동기화 체크박스)
        ImGui::Checkbox("한계 돌파 모드 (1~255 허용)", &m_limitBreakMode);
        ImGui::SameLine();
        ImGui::Checkbox("실시간 자동 핫싱크 (Auto-Sync)", &m_autoSync);

        ImGui::Spacing();

        int maxStat = m_limitBreakMode ? 255 : 100;
        bool valueChanged = false;

        if (ImGui::BeginTabBar("OfficerInspectorTabs", ImGuiTabBarFlags_None)) {
            // ----------------------------------------------------
            // 탭 1: 능력치 및 적성 (Stats & Aptitudes)
            // ----------------------------------------------------
            if (ImGui::BeginTabItem("능력치 및 적성##TabStats")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 4대 핵심 능력치 (통솔, 무력, 지력, 정치)");

                int lead = m_editStats.curr_leadership;
                int war  = m_editStats.curr_war;
                int intel= m_editStats.curr_intel;
                int pol  = m_editStats.curr_politics;

                ImGui::PushItemWidth(300.0f);
                if (ImGui::SliderInt("통솔 (Leadership)##EditLead", &lead, 1, maxStat)) {
                    m_editStats.base_leadership = lead;
                    m_editStats.curr_leadership = lead;
                    valueChanged = true;
                }
                if (ImGui::SliderInt("무력 (War)##EditWar", &war, 1, maxStat)) {
                    m_editStats.base_war = war;
                    m_editStats.curr_war = war;
                    valueChanged = true;
                }
                if (ImGui::SliderInt("지력 (Intel)##EditIntel", &intel, 1, maxStat)) {
                    m_editStats.base_intel = intel;
                    m_editStats.curr_intel = intel;
                    valueChanged = true;
                }
                if (ImGui::SliderInt("정치 (Politics)##EditPol", &pol, 1, maxStat)) {
                    m_editStats.base_politics = pol;
                    m_editStats.curr_politics = pol;
                    valueChanged = true;
                }
                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 병종 적성
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 병종 적성 (Strict Guard: 0=C, 1=B, 2=A, 3=S)");

                const char* aptItems[] = { "C (0)", "B (1)", "A (2)", "S (3)" };
                int spear = m_editStats.spear_apt & 0x03;
                int horse = m_editStats.horse_apt & 0x03;
                int bow   = m_editStats.bow_apt   & 0x03;

                ImGui::PushItemWidth(120.0f);
                if (ImGui::Combo("창병##EditSpear", &spear, aptItems, 4)) {
                    m_editStats.spear_apt = spear;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Combo("기병##EditHorse", &horse, aptItems, 4)) {
                    m_editStats.horse_apt = horse;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Combo("궁병##EditBow", &bow, aptItems, 4)) {
                    m_editStats.bow_apt = bow;
                    valueChanged = true;
                }
                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 생몰년 및 수명
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 생몰년 및 수명");
                ImGui::Text("출생: %d년 | 등장: %d년 | 사망: %d년",
                            m_editingRecord.birth_year, m_editingRecord.appear_year, m_editingRecord.death_year);
                ImGui::SameLine();
                if (ImGui::Button("+5년 수명 연장")) {
                    m_officerMgr->ExtendLife(m_editingRecord.id, 5);
                    m_editingRecord.death_year += 5;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("+10년 수명 연장")) {
                    m_officerMgr->ExtendLife(m_editingRecord.id, 10);
                    m_editingRecord.death_year += 10;
                    valueChanged = true;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 빠른 프리셋 치트 버튼
                ImGui::Text("빠른 치트 프리셋:");
                ImGui::SameLine();
                if (ImGui::Button("올 100")) {
                    m_editStats.base_leadership = 100; m_editStats.curr_leadership = 100;
                    m_editStats.base_war = 100;        m_editStats.curr_war = 100;
                    m_editStats.base_intel = 100;      m_editStats.curr_intel = 100;
                    m_editStats.base_politics = 100;   m_editStats.curr_politics = 100;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("올 255 (패왕)")) {
                    m_limitBreakMode = true;
                    m_editStats.base_leadership = 255; m_editStats.curr_leadership = 255;
                    m_editStats.base_war = 255;        m_editStats.curr_war = 255;
                    m_editStats.base_intel = 255;      m_editStats.curr_intel = 255;
                    m_editStats.base_politics = 255;   m_editStats.curr_politics = 255;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("전종 S급")) {
                    m_editStats.spear_apt = 3;
                    m_editStats.horse_apt = 3;
                    m_editStats.bow_apt   = 3;
                    valueChanged = true;
                }

                ImGui::EndTabItem();
            }

            // ----------------------------------------------------
            // 탭 2: 전법 및 20대 특기 (Tactics & 20 Skills)
            // ----------------------------------------------------
            if (ImGui::BeginTabItem("전법 및 20대 특기##TabSkills")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 보유 전법 (Tactics, 0~140)");
                int tacticId = m_editingRecord.tactic_id;
                ImGui::PushItemWidth(260.0f);
                if (ImGui::BeginCombo("전법 선택##EditTactic", OfficerRecord::GetTacticName(tacticId))) {
                    for (int i = 0; i < 141; ++i) {
                        bool isSel = (tacticId == i);
                        char itemLabel[64];
                        std::snprintf(itemLabel, sizeof(itemLabel), "%03d: %s", i, OfficerRecord::GetTacticName(i));
                        if (ImGui::Selectable(itemLabel, isSel)) {
                            m_editingRecord.tactic_id = static_cast<uint8_t>(i);
                            valueChanged = true;
                        }
                        if (isSel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();

                ImGui::SameLine();
                ImGui::TextDisabled("(현재: %s)", OfficerRecord::GetTacticName(m_editingRecord.tactic_id));

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 20대 특기 레벨 (0~9 레벨)");
                ImGui::SameLine();
                if (ImGui::SmallButton("전체 Lv 9 MAX")) {
                    for (int i = 0; i < 20; ++i) m_editingRecord.skills[i] = 9;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("전체 Lv 0 초기화")) {
                    for (int i = 0; i < 20; ++i) m_editingRecord.skills[i] = 0;
                    valueChanged = true;
                }

                ImGui::Columns(2, "SkillColumns", false);
                for (int i = 0; i < 20; ++i) {
                    int val = m_editingRecord.skills[i];
                    char sLabel[32];
                    std::snprintf(sLabel, sizeof(sLabel), "%s##Skill_%d", OfficerRecord::GetSkillName(i), i);
                    ImGui::PushItemWidth(140.0f);
                    if (ImGui::SliderInt(sLabel, &val, 0, 9)) {
                        m_editingRecord.skills[i] = static_cast<uint8_t>(val);
                        valueChanged = true;
                    }
                    ImGui::PopItemWidth();
                    if (i == 9) ImGui::NextColumn();
                }
                ImGui::Columns(1);

                ImGui::EndTabItem();
            }

            // ----------------------------------------------------
            // 탭 3: 외모·성격·가치관·기호 (Character & Values)
            // ----------------------------------------------------
            if (ImGui::BeginTabItem("외모·성격·가치관·기호##TabCharacter")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 외모 및 성격");

                int face = m_editingRecord.face_id;
                ImGui::PushItemWidth(150.0f);
                if (ImGui::InputInt("얼굴 CG (Face ID)##EditFace", &face)) {
                    if (face < 0) face = 0;
                    m_editingRecord.face_id = static_cast<uint16_t>(face);
                    valueChanged = true;
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::TextDisabled("(조조: 405, 관우: 117, 제갈량: 347, 여포: 786, 장비: 517)");

                int pers = m_editingRecord.personality;
                ImGui::Text("성격:");
                ImGui::SameLine();
                if (ImGui::RadioButton("소심(0)", &pers, 0)) { m_editingRecord.personality = 0; valueChanged = true; }
                ImGui::SameLine();
                if (ImGui::RadioButton("냉정(1)", &pers, 1)) { m_editingRecord.personality = 1; valueChanged = true; }
                ImGui::SameLine();
                if (ImGui::RadioButton("대담(2)", &pers, 2)) { m_editingRecord.personality = 2; valueChanged = true; }
                ImGui::SameLine();
                if (ImGui::RadioButton("저돌(3)", &pers, 3)) { m_editingRecord.personality = 3; valueChanged = true; }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 4대 기호 (관심사 및 주량)");
                bool likeW = (m_editingRecord.character.like_weapon != 0);
                bool likeB = (m_editingRecord.character.like_book != 0);
                bool likeT = (m_editingRecord.character.like_treasure != 0);

                if (ImGui::Checkbox("선호 무기##LikeWeapon", &likeW)) {
                    m_editingRecord.character.like_weapon = likeW ? 1 : 0;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("선호 서적##LikeBook", &likeB)) {
                    m_editingRecord.character.like_book = likeB ? 1 : 0;
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("선호 보물##LikeTreasure", &likeT)) {
                    m_editingRecord.character.like_treasure = likeT ? 1 : 0;
                    valueChanged = true;
                }

                const char* drinkItems[] = { "금주 (0)", "보통 (1)", "대음 (2)" };
                int drink = m_editingRecord.character.drink_level;
                if (drink > 2) drink = 2;
                ImGui::PushItemWidth(150.0f);
                if (ImGui::Combo("음주 / 주량##EditDrink", &drink, drinkItems, 3)) {
                    m_editingRecord.character.drink_level = static_cast<uint8_t>(drink);
                    valueChanged = true;
                }
                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 4대 가치관 및 상성");
                const char* greedItems[] = { "무욕 (0)", "보통 (1)", "강욕 (2)" };
                int greed = m_editingRecord.character.greed;
                if (greed > 2) greed = 2;
                ImGui::PushItemWidth(140.0f);
                if (ImGui::Combo("물욕##EditGreed", &greed, greedItems, 3)) {
                    m_editingRecord.character.greed = static_cast<uint8_t>(greed);
                    valueChanged = true;
                }

                const char* valItems[] = { "경시 (0)", "낮음 (1)", "보통 (2)", "중시 (3)", "매우중시 (4)" };
                int loy = m_editingRecord.character.loyalty_justice; if (loy > 4) loy = 4;
                int cou = m_editingRecord.character.valuing_courage; if (cou > 4) cou = 4;
                int tal = m_editingRecord.character.valuing_talent;  if (tal > 4) tal = 4;

                if (ImGui::Combo("의리##EditLoy", &loy, valItems, 5)) {
                    m_editingRecord.character.loyalty_justice = static_cast<uint8_t>(loy);
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Combo("용애##EditCou", &cou, valItems, 5)) {
                    m_editingRecord.character.valuing_courage = static_cast<uint8_t>(cou);
                    valueChanged = true;
                }
                if (ImGui::Combo("재애##EditTal", &tal, valItems, 5)) {
                    m_editingRecord.character.valuing_talent = static_cast<uint8_t>(tal);
                    valueChanged = true;
                }

                int aff = m_editingRecord.character.affinity;
                if (ImGui::SliderInt("상성 (Affinity)##EditAff", &aff, 0, 149)) {
                    m_editingRecord.character.affinity = static_cast<uint8_t>(aff);
                    valueChanged = true;
                }
                ImGui::PopItemWidth();

                ImGui::EndTabItem();
            }

            // ----------------------------------------------------
            // 탭 4: 가족 및 인연 (Family & Bonds)
            // ----------------------------------------------------
            if (ImGui::BeginTabItem("가족 및 인연##TabBonds")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 가족 및 혈통 (0xFFFF = 미지정)");

                auto DrawOfficerIdSelector = [&](const char* label, uint16_t& targetId) {
                    int idVal = (targetId == 0xFFFF) ? -1 : targetId;
                    ImGui::PushItemWidth(140.0f);
                    char inputLabel[64];
                    std::snprintf(inputLabel, sizeof(inputLabel), "%s##Id", label);
                    if (ImGui::InputInt(inputLabel, &idVal)) {
                        targetId = (idVal < 0 || idVal >= 2165) ? 0xFFFF : static_cast<uint16_t>(idVal);
                        valueChanged = true;
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    if (targetId != 0xFFFF) {
                        std::string_view name = GetOfficerKoreanName(targetId);
                        if (!name.empty()) {
                            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%s (ID %d)", std::string(name).c_str(), targetId);
                        } else {
                            ImGui::TextDisabled("무장 #%d", targetId);
                        }
                    } else {
                        ImGui::TextDisabled("(없음)");
                    }
                };

                DrawOfficerIdSelector("부친 무장", m_editingRecord.father_id);
                DrawOfficerIdSelector("모친 무장", m_editingRecord.mother_id);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 배우자 (최대 3인)");
                DrawOfficerIdSelector("배우자 1", m_editingRecord.spouse_1_id);
                DrawOfficerIdSelector("배우자 2", m_editingRecord.spouse_2_id);
                DrawOfficerIdSelector("배우자 3", m_editingRecord.spouse_3_id);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 특수 인연");
                DrawOfficerIdSelector("의형제 / 단짝", m_editingRecord.sworn_brother_id);
                DrawOfficerIdSelector("원수 무장", m_editingRecord.rival_id);

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // 실시간 자동 동기화 활성화 시 값 변경 즉시 커밋
        if (valueChanged && m_autoSync) {
            CommitEditToMemory();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 6. 하단 상태 바 및 조작 버튼
        if (!m_syncStatusMsg.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%s", m_syncStatusMsg.c_str());
        } else {
            ImGui::TextDisabled("동기화 대기 중...");
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 250.0f);
        if (ImGui::Button("메모리 적용 (Hot-Sync)", ImVec2(170, 0))) {
            CommitEditToMemory();
        }
        ImGui::SameLine();
        if (ImGui::Button("닫기", ImVec2(60, 0))) {
            m_showEditModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void OfficerTableView::RenderBatchEditModal() {
    if (m_showBatchModal && !ImGui::IsPopupOpen("무장 일괄 수정 (Batch Action)##OfficerBatchModal")) {
        ImGui::OpenPopup("무장 일괄 수정 (Batch Action)##OfficerBatchModal");
    }

    if (ImGui::BeginPopupModal("무장 일괄 수정 (Batch Action)##OfficerBatchModal", &m_showBatchModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        size_t selCount = m_selectedOfficerIds.size();

        // 1. 헤더 정보 및 대상 무장 요약
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "■ 무장 일괄 수정 (Batch Action) - 총 %zu명 선택됨", selCount);

        std::string sampleNames;
        size_t displayed = 0;
        for (const auto& off : m_cachedOfficers) {
            if (m_selectedOfficerIds.find(off.id) != m_selectedOfficerIds.end()) {
                if (displayed > 0) sampleNames += ", ";
                sampleNames += off.name;
                displayed++;
                if (displayed >= 5) break;
            }
        }
        if (selCount > displayed) {
            sampleNames += " ... 외 " + std::to_string(selCount - displayed) + "명";
        }
        ImGui::TextDisabled("대상: %s", sampleNames.c_str());

        ImGui::Separator();
        ImGui::Spacing();

        // 제어 옵션: 한계 돌파 모드
        ImGui::Checkbox("한계 돌파 모드 (1~255 스탯 허용)", &m_limitBreakMode);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 섹션 1: 능력치 일괄 가산 / 감산 (Buff / Debuff)
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 1. 능력치 일괄 가산/감산 (아군 스탯 버프 / 디버프)");
        ImGui::PushItemWidth(140.0f);
        ImGui::SliderInt("통솔 +Δ##bLead", &m_batchDeltaLead, -50, 50);
        ImGui::SameLine();
        ImGui::SliderInt("무력 +Δ##bWar", &m_batchDeltaWar, -50, 50);
        ImGui::SliderInt("지력 +Δ##bIntel", &m_batchDeltaIntel, -50, 50);
        ImGui::SameLine();
        ImGui::SliderInt("정치 +Δ##bPol", &m_batchDeltaPol, -50, 50);
        ImGui::PopItemWidth();

        if (ImGui::Button("선택 무장 스탯 일괄 가산 적용 (Buff)", ImVec2(280, 0))) {
            ApplyBatchStatsDelta(m_batchDeltaLead, m_batchDeltaWar, m_batchDeltaIntel, m_batchDeltaPol);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 섹션 2: 수명 및 생몰년도 일괄 연장
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 2. 수명 일괄 연장 (사망년도 +N년)");
        ImGui::PushItemWidth(180.0f);
        ImGui::SliderInt("연장 연수 (+년)##bLife", &m_batchLifeYears, 1, 100);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("수명 일괄 연장 적용", ImVec2(180, 0))) {
            ApplyBatchLifeExtension(m_batchLifeYears);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 섹션 3: 병종 적성 일괄 승급
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 3. 병종 적성 일괄 승급");
        const char* aptNames[] = { "C (0)", "B (1)", "A (2)", "S (3)" };
        ImGui::PushItemWidth(90.0f);
        ImGui::Combo("창병##bSpear", &m_batchSpearApt, aptNames, 4);
        ImGui::SameLine();
        ImGui::Combo("기병##bHorse", &m_batchHorseApt, aptNames, 4);
        ImGui::SameLine();
        ImGui::Combo("궁병##bBow", &m_batchBowApt, aptNames, 4);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("적성 일괄 적용")) {
            ApplyBatchAptitudes(m_batchSpearApt, m_batchHorseApt, m_batchBowApt);
        }
        ImGui::SameLine();
        if (ImGui::Button("전종 S급 일괄 승급")) {
            BatchCheatAllAptitudeS();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 섹션 4: 빠른 일괄 치트 프리셋
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 4. 빠른 일괄 치트 프리셋");
        if (ImGui::Button("아군 올스탯 +10 버프")) {
            BatchCheatAllBuff10();
        }
        ImGui::SameLine();
        if (ImGui::Button("선택 무장 전원 올 100")) {
            BatchCheatAll100();
        }
        ImGui::SameLine();
        if (ImGui::Button("전원 올 255 (한계돌파)")) {
            BatchCheatAll255();
        }
        ImGui::SameLine();
        if (ImGui::Button("수명 +30년 연장")) {
            BatchCheatLifeExt30();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 섹션 5: 트랜잭션 상태 및 롤백 (실행 취소 / Undo)
        if (!m_batchStatusMsg.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%s", m_batchStatusMsg.c_str());
        } else {
            ImGui::TextDisabled("일괄 작업 대기 중...");
        }

        if (HasUndoSnapshot()) {
            ImGui::SameLine();
            char undoBtn[128];
            std::snprintf(undoBtn, sizeof(undoBtn), "직전 일괄 수정 실행 취소 (Rollback %zu명) ↺", m_lastBatchSnapshots.size());
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button(undoBtn)) {
                UndoLastBatchOperation();
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 90.0f);
        if (ImGui::Button("닫기", ImVec2(80, 0))) {
            m_showBatchModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace San13::UI
