#include "ui/CityDetailModal.hpp"
#include "game/ForceManager.hpp"
#include "imgui.h"
#include <chrono>
#include <cstdio>
#include <algorithm>

namespace San13::UI {

CityDetailModal::CityDetailModal()
    : m_cityMgr(nullptr), m_setMgr(nullptr), m_isOpen(false), m_cityId(-1) {
}

CityDetailModal::~CityDetailModal() = default;

void CityDetailModal::Initialize(Game::CityManager* cityMgr, Game::SettlementManager* setMgr) {
    m_cityMgr = cityMgr;
    m_setMgr = setMgr;
}

void CityDetailModal::Open(int cityId) {
    m_cityId = cityId;
    m_isOpen = true;
    m_syncStatusMsg.clear();
    RefreshData();
}

void CityDetailModal::Close() {
    m_isOpen = false;
}

void CityDetailModal::RefreshData() {
    if (!m_cityMgr || m_cityId < 0 || m_cityId >= Game::CityMemoryLayout::MAX_CITY_COUNT) {
        return;
    }

    m_cityMgr->LoadCity(static_cast<uint8_t>(m_cityId), m_record);

    m_settlements.clear();
    if (m_setMgr) {
        m_setMgr->LoadCitySettlements(static_cast<uint8_t>(m_cityId),
                                      m_record.name,
                                      m_settlements);
    }

    m_cityMgr->LoadAllCityOfficers(static_cast<uint8_t>(m_cityId),
                                   m_activeOfficers,
                                   m_prisoners,
                                   m_freeOfficers);
}

bool CityDetailModal::CommitEditToMemory() {
    if (!m_cityMgr || m_cityId < 0 || m_cityId >= Game::CityMemoryLayout::MAX_CITY_COUNT) {
        return false;
    }

    auto start = std::chrono::high_resolution_clock::now();
    Game::ClampingPolicy policy = m_limitBreakMode ? Game::ClampingPolicy::LimitBreak : Game::ClampingPolicy::SafeVanilla;

    uint8_t cid = static_cast<uint8_t>(m_cityId);
    bool ok1 = m_cityMgr->SetCityGold(cid, m_record.gold, policy);
    bool ok2 = m_cityMgr->SetCityProvisions(cid, m_record.provisions, policy);
    bool ok3 = m_cityMgr->SetCityTroops(cid, m_record.troops, policy);
    bool ok4 = m_cityMgr->SetCityDurability(cid, m_record.durability, policy);

    // Domestic setters
    bool ok5 = m_cityMgr->SetCityPublicOrder(cid, m_record.public_order, policy);
    bool ok6 = m_cityMgr->SetCityCommerce(cid, m_record.commerce, policy);
    bool ok7 = m_cityMgr->SetCityAgriculture(cid, m_record.agriculture, policy);
    bool ok8 = m_cityMgr->SetCityCulture(cid, m_record.culture, policy);

    // Proficiency setters
    bool ok9 = m_cityMgr->SetCitySpearProficiency(cid, m_record.spear_proficiency, policy);
    bool ok10 = m_cityMgr->SetCityHorseProficiency(cid, m_record.horse_proficiency, policy);
    bool ok11 = m_cityMgr->SetCityBowProficiency(cid, m_record.bow_proficiency, policy);

    // Settlement setters
    bool ok12 = true;
    if (m_setMgr) {
        for (const auto& s : m_settlements) {
            if (!m_setMgr->SetSettlementDevProgress(s.id, s.dev_progress, cid)) {
                ok12 = false;
            }
            m_setMgr->SetSettlementMaxLevel(s.id, s.max_fortress_level);
            if (s.fortress_type != Game::FortressType::None) {
                m_setMgr->FortifySettlement(s.id, s.fortress_type, s.hp, cid);
            } else if (!s.is_fortified) {
                m_setMgr->DemolishFortress(s.id);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    m_lastSyncTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

    // 동기화 후 최신 수치 다시 로드 (클램핑 확인)
    m_cityMgr->LoadCity(cid, m_record);
    if (m_setMgr) {
        m_setMgr->LoadCitySettlements(cid, m_record.name, m_settlements);
    }

    bool allOk = ok1 && ok2 && ok3 && ok4 && ok5 && ok6 && ok7 && ok8 && ok9 && ok10 && ok11 && ok12;
    char buf[128];
    if (allOk) {
        std::snprintf(buf, sizeof(buf), "● 실시간 핫싱크 성공 (%.3fms)", m_lastSyncTimeMs);
    } else {
        std::snprintf(buf, sizeof(buf), "▲ 일부 데이터 커밋 실패 (%.3fms)", m_lastSyncTimeMs);
    }
    m_syncStatusMsg = buf;

    return allOk;
}

bool CityDetailModal::QuickCheatMaxResources() {
    m_record.gold = 999999;
    m_record.provisions = 999999;
    m_record.durability = 5000;
    return CommitEditToMemory();
}

bool CityDetailModal::QuickCheatDepleteResources() {
    m_record.gold = 0;
    m_record.provisions = 0;
    m_record.troops = 0;
    return CommitEditToMemory();
}

bool CityDetailModal::QuickCheatMaxDomestic() {
    m_record.public_order = 1000;
    m_record.commerce = 3000;
    m_record.agriculture = 3000;
    m_record.culture = 3000;
    return CommitEditToMemory();
}

bool CityDetailModal::QuickCheatMaxSettlements() {
    if (m_setMgr) {
        uint8_t cid = static_cast<uint8_t>(m_cityId);
        m_setMgr->MaximizeCitySettlements(cid);
        for (auto& s : m_settlements) {
            s.dev_progress = 100;
        }
    }
    return CommitEditToMemory();
}

bool CityDetailModal::QuickCheatFortifyAllSettlements() {
    if (m_setMgr) {
        uint8_t cid = static_cast<uint8_t>(m_cityId);
        m_setMgr->FortifyAllCitySettlements(cid, Game::FortressType::Stronghold);
        for (auto& s : m_settlements) {
            s.dev_progress = 100;
            s.max_fortress_level = 5;
            s.fortress_type = Game::FortressType::Stronghold;
            s.current_fortress_level = 5;
            s.is_fortified = true;
            s.hp = 2500;
            s.max_hp = 2500;
            s.type_tag = "[방채]";
        }
    }
    return CommitEditToMemory();
}

bool CityDetailModal::QuickCheatMaxProficiencies() {
    m_record.spear_proficiency = 3000;
    m_record.horse_proficiency = 3000;
    m_record.bow_proficiency = 3000;
    return CommitEditToMemory();
}

void CityDetailModal::Render() {
    if (m_isOpen && !ImGui::IsPopupOpen("도시 종합 인스펙터##CityDetailModal")) {
        ImGui::OpenPopup("도시 종합 인스펙터##CityDetailModal");
    }

    ImGui::SetNextWindowSize(ImVec2(820, 580), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(720, 480), ImVec2(1200, 750));

    if (ImGui::BeginPopupModal("도시 종합 인스펙터##CityDetailModal", &m_isOpen, ImGuiWindowFlags_NoCollapse)) {
        // 상단 헤더
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "■ %s [ID %02d]",
                           m_record.name.c_str(), m_record.id);
        ImGui::SameLine();
        ImGui::TextDisabled("| 소속: %s (군주: %s) | 총인구: %s명",
                            m_record.force_name.c_str(),
                            m_record.ruler_name.c_str(),
                            std::to_string(m_record.GetTotalPopulation()).c_str());

        ImGui::Separator();
        ImGui::Spacing();

        // 4탭 바 렌더링
        if (ImGui::BeginTabBar("CityDetailTabBar", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("기본 경제/자원 (Economy)")) {
                RenderTabEconomy();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("내정 및 관할 부락 (Domestic)")) {
                RenderTabDomestic();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("군사 및 숙련도 (Military)")) {
                RenderTabMilitary();
                ImGui::EndTabItem();
            }
            char officerTabTitle[64];
            std::snprintf(officerTabTitle, sizeof(officerTabTitle),
                          "도시 소속 인물 (%zu명)##TabOff", m_activeOfficers.size());
            if (ImGui::BeginTabItem(officerTabTitle)) {
                RenderTabOfficers();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 하단 상태 표시줄 및 버튼 영역
        if (!m_syncStatusMsg.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%s", m_syncStatusMsg.c_str());
        } else {
            ImGui::TextDisabled("핫싱크 대기 중...");
        }

        float buttonWidth = 140.0f;
        float rightPad = 10.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - (buttonWidth * 2 + 80.0f + rightPad));

        if (ImGui::Button("메모리 적용 (Hot-Sync)", ImVec2(buttonWidth, 0))) {
            CommitEditToMemory();
        }
        ImGui::SameLine();
        if (ImGui::Button("새로고침 (Refresh)", ImVec2(buttonWidth, 0))) {
            RefreshData();
        }
        ImGui::SameLine();
        if (ImGui::Button("닫기", ImVec2(70, 0))) {
            m_isOpen = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void CityDetailModal::RenderTabEconomy() {
    ImGui::Spacing();

    // 옵션 체크박스
    ImGui::Checkbox("한계 돌파 모드 (자금/군량 9,999,999 허용)", &m_limitBreakMode);
    ImGui::SameLine(360.0f);
    ImGui::Checkbox("실시간 자동 동기화 (Auto-Sync)", &m_autoSync);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    int maxGold = m_limitBreakMode ? 9999999 : 999999;
    int maxProv = m_limitBreakMode ? 9999999 : 999999;
    int maxTroops = m_limitBreakMode ? 999999 : 200000;
    int maxDur = m_limitBreakMode ? 10000 : 5000;
    bool valueChanged = false;

    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 도시 경제 및 주둔 병력 수치");

    int gold = static_cast<int>(m_record.gold);
    int prov = static_cast<int>(m_record.provisions);
    int troops = static_cast<int>(m_record.troops);
    int dur = static_cast<int>(m_record.durability);

    ImGui::PushItemWidth(300.0f);
    if (ImGui::SliderInt("자금 (Gold)##EditGold", &gold, 0, maxGold)) {
        m_record.gold = gold;
        valueChanged = true;
    }
    if (ImGui::SliderInt("군량 (Provisions)##EditProv", &prov, 0, maxProv)) {
        m_record.provisions = prov;
        valueChanged = true;
    }
    if (ImGui::SliderInt("주둔 병력 (Troops)##EditTroops", &troops, 0, maxTroops)) {
        m_record.troops = troops;
        valueChanged = true;
    }
    if (ImGui::SliderInt("성벽 내구도 (Durability)##EditDur", &dur, 0, maxDur)) {
        m_record.durability = dur;
        valueChanged = true;
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 인구 분석 카드
    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "▶ 인구 구성 분석 (Step 2.2 공식 검증)");
    ImGui::BulletText("기초 민간 인구: %u 명 (+0x60)", m_record.civilian_population);
    ImGui::BulletText("주둔 병력: %u 명 (+0x64)", m_record.troops);
    ImGui::BulletText("합산 총 인구: %u 명 (민간 인구 + 주둔 병력)", m_record.GetTotalPopulation());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 빠른 치트 버튼 프리셋
    ImGui::Text("빠른 치트 프리셋:");
    ImGui::SameLine();
    if (ImGui::Button("자금/군량 999,999 완충")) {
        m_record.gold = 999999;
        m_record.provisions = 999999;
        valueChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("병력 100,000 증원")) {
        m_record.troops = std::min<uint32_t>(m_record.troops + 100000, maxTroops);
        valueChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("내구도 5,000 완충")) {
        m_record.durability = 5000;
        valueChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("자원 고갈 (자금/군량 0)")) {
        m_record.gold = 0;
        m_record.provisions = 0;
        m_record.troops = 0;
        valueChanged = true;
    }

    if (valueChanged && m_autoSync) {
        CommitEditToMemory();
    }
}

void CityDetailModal::RenderTabDomestic() {
    bool valueChanged = false;
    int maxOrder = m_limitBreakMode ? Game::StatBounds::CityPublicOrder::LIMIT_BREAK_MAX : Game::StatBounds::CityPublicOrder::VANILLA_MAX;
    int maxDomestic = m_limitBreakMode ? Game::StatBounds::CityDomestic::LIMIT_BREAK_MAX : Game::StatBounds::CityDomestic::VANILLA_MAX;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 도시 내정 기본치 편집 (실시간 핫싱크 지원)");

    ImGui::Columns(4, "DomesticCols", false);

    // 민심
    ImGui::Text("민심 (Public Order)");
    int order = static_cast<int>(m_record.public_order);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##OrderSlider", &order, 0, maxOrder)) {
        m_record.public_order = static_cast<uint16_t>(order);
        valueChanged = true;
    }
    float orderRatio = std::min(1.0f, m_record.public_order / 1000.0f);
    char orderBuf[32];
    std::snprintf(orderBuf, sizeof(orderBuf), "%u / 1000", m_record.public_order);
    ImGui::ProgressBar(orderRatio, ImVec2(-1, 0), orderBuf);
    ImGui::NextColumn();

    // 상업
    ImGui::Text("상업 (Commerce)");
    int comm = static_cast<int>(m_record.commerce);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##CommSlider", &comm, 0, maxDomestic)) {
        m_record.commerce = static_cast<uint16_t>(comm);
        valueChanged = true;
    }
    float commRatio = std::min(1.0f, m_record.commerce / 3000.0f);
    char commBuf[32];
    std::snprintf(commBuf, sizeof(commBuf), "%u", m_record.commerce);
    ImGui::ProgressBar(commRatio, ImVec2(-1, 0), commBuf);
    ImGui::NextColumn();

    // 농업
    ImGui::Text("농업 (Agriculture)");
    int agri = static_cast<int>(m_record.agriculture);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##AgriSlider", &agri, 0, maxDomestic)) {
        m_record.agriculture = static_cast<uint16_t>(agri);
        valueChanged = true;
    }
    float agriRatio = std::min(1.0f, m_record.agriculture / 3000.0f);
    char agriBuf[32];
    std::snprintf(agriBuf, sizeof(agriBuf), "%u", m_record.agriculture);
    ImGui::ProgressBar(agriRatio, ImVec2(-1, 0), agriBuf);
    ImGui::NextColumn();

    // 문화
    ImGui::Text("문화 (Culture)");
    int cult = static_cast<int>(m_record.culture);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##CultSlider", &cult, 0, maxDomestic)) {
        m_record.culture = static_cast<uint16_t>(cult);
        valueChanged = true;
    }
    float cultRatio = std::min(1.0f, m_record.culture / 3000.0f);
    char cultBuf[32];
    std::snprintf(cultBuf, sizeof(cultBuf), "%u", m_record.culture);
    ImGui::ProgressBar(cultRatio, ImVec2(-1, 0), cultBuf);
    ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Text("빠른 내정 치트:");
    ImGui::SameLine();
    if (ImGui::Button("내정 올 3,000 완충")) {
        QuickCheatMaxDomestic();
    }
    ImGui::SameLine();
    if (ImGui::Button("관할 부락 100% 일괄 장악")) {
        QuickCheatMaxSettlements();
    }
    ImGui::SameLine();
    if (ImGui::Button("관할 부락 전체 방채(Lv5) 요새화")) {
        QuickCheatFortifyAllSettlements();
    }
    if (m_limitBreakMode) {
        ImGui::SameLine();
        if (ImGui::Button("한계 돌파 (올 10,000 / 민심 5,000)")) {
            m_record.public_order = 5000;
            m_record.commerce = 10000;
            m_record.agriculture = 10000;
            m_record.culture = 10000;
            valueChanged = true;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 관할 부락 현황 및 개별 편집
    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f),
                       "▶ 관할 부락 현황 및 성채(건물) 건축/요충 레벨 편집 (총 %u개 등록 / %zu개 로드)",
                       m_record.settlement_count, m_settlements.size());

    if (m_settlements.empty()) {
        ImGui::TextDisabled("관할 부락이 없거나 로드되지 않았습니다.");
    } else {
        static ImGuiTableFlags setTableFlags =
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

        if (ImGui::BeginTable("SettlementDetailTable", 8, setTableFlags)) {
            ImGui::TableSetupColumn("부락 ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("유형", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("최대 Lv", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("부락명", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("효과 / 특수병과", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("지배 세력", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("장악도 (0~100%)", ImGuiTableColumnFlags_WidthFixed, 135.0f);
            ImGui::TableSetupColumn("성채 건축 / 개축 (내구도)", ImGuiTableColumnFlags_WidthFixed, 235.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < m_settlements.size(); ++i) {
                auto& s = m_settlements[i];
                ImGui::TableNextRow();

                // 0: 부락 ID
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("#%03u", s.id);

                // 1: 거점 유형 태그 (색상 뱃지)
                ImGui::TableSetColumnIndex(1);
                if (s.fortress_type == Game::FortressType::Stronghold) {
                    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", s.type_tag.c_str());
                } else if (s.fortress_type == Game::FortressType::SupplyDepot) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "%s", s.type_tag.c_str());
                } else if (s.fortress_type == Game::FortressType::Camp) {
                    ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "%s", s.type_tag.c_str());
                } else if (s.is_fortified) {
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "%s", s.type_tag.c_str());
                } else {
                    ImGui::TextDisabled("%s", s.type_tag.c_str());
                }

                // 2: 최대 건물 Lv 및 Lv 5 승급 치트
                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(static_cast<int>(s.id * 10 + 3));
                if (s.max_fortress_level >= 5) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Lv 5");
                } else {
                    ImGui::Text("Lv %u", s.max_fortress_level);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Lv5")) {
                        s.max_fortress_level = 5;
                        valueChanged = true;
                    }
                }
                ImGui::PopID();

                // 3: 부락명
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", s.name.c_str());

                // 4: 효과 / 특수병과
                ImGui::TableSetColumnIndex(4);
                if (!s.effect_name.empty() && s.effect_name != "-") {
                    ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "%s", s.effect_name.c_str());
                } else {
                    ImGui::TextDisabled("-");
                }

                // 5: 지배 세력
                ImGui::TableSetColumnIndex(5);
                if (s.force_name.find("미종속") != std::string::npos || s.force_name.find("공백지") != std::string::npos) {
                    ImGui::TextDisabled("%s", s.force_name.c_str());
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "%s", s.force_name.c_str());
                }

                // 6: 장악도
                ImGui::TableSetColumnIndex(6);
                ImGui::PushID(static_cast<int>(s.id * 10 + 1));
                int dev = static_cast<int>(s.dev_progress);
                ImGui::SetNextItemWidth(70.0f);
                if (ImGui::SliderInt("##dev", &dev, 0, 100, "%d%%")) {
                    s.dev_progress = static_cast<uint16_t>(dev);
                    valueChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("100%")) {
                    s.dev_progress = 100;
                    valueChanged = true;
                }
                ImGui::PopID();

                // 7: 성채 건축 / 개축 (내구도)
                ImGui::TableSetColumnIndex(7);
                ImGui::PushID(static_cast<int>(s.id * 10 + 2));

                const char* fortressItems[] = {
                    "미건축 (부락)",
                    "견진 (Lv1, 1000)",
                    "군악대 (Lv2, 1500)",
                    "요양사 (Lv3, 1500)",
                    "병량고 (Lv4, 2000)",
                    "방채 (Lv5, 2500)"
                };
                int currentFort = static_cast<int>(s.fortress_type);
                if (currentFort < 0 || currentFort > 5) currentFort = 0;

                ImGui::SetNextItemWidth(135.0f);
                if (ImGui::Combo("##fortCombo", &currentFort, fortressItems, IM_ARRAYSIZE(fortressItems))) {
                    s.fortress_type = static_cast<Game::FortressType>(currentFort);
                    s.is_fortified = (s.fortress_type != Game::FortressType::None);
                    s.current_fortress_level = currentFort;
                    s.type_tag = Game::SettlementManager::GetFortressTag(s.fortress_type);

                    // 필요 레벨이 현재 부락 최대 레벨보다 크면 자동으로 승급
                    if (currentFort > s.max_fortress_level) {
                        s.max_fortress_level = currentFort;
                    }

                    switch (s.fortress_type) {
                        case Game::FortressType::Camp:        s.max_hp = 1000; s.hp = 1000; break;
                        case Game::FortressType::MusicTower:  s.max_hp = 1500; s.hp = 1500; break;
                        case Game::FortressType::Hospital:    s.max_hp = 1500; s.hp = 1500; break;
                        case Game::FortressType::SupplyDepot: s.max_hp = 2000; s.hp = 2000; break;
                        case Game::FortressType::Stronghold:  s.max_hp = 2500; s.hp = 2500; break;
                        default:                              s.max_hp = 0;    s.hp = 0;    break;
                    }
                    if (s.is_fortified) {
                        s.dev_progress = 100; // 건물 건축 시 100% 자동 장악
                    }
                    valueChanged = true;
                }

                if (s.is_fortified && s.max_hp > 0) {
                    ImGui::SameLine();
                    float hpRatio = static_cast<float>(s.hp) / static_cast<float>(s.max_hp);
                    char hpBuf[32];
                    std::snprintf(hpBuf, sizeof(hpBuf), "%u/%u", s.hp, s.max_hp);
                    ImGui::ProgressBar(hpRatio, ImVec2(80.0f, 0), hpBuf);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    if (valueChanged && m_autoSync) {
        CommitEditToMemory();
    }
}

void CityDetailModal::RenderTabMilitary() {
    bool valueChanged = false;
    int maxProf = m_limitBreakMode ? Game::StatBounds::CityProficiency::LIMIT_BREAK_MAX : Game::StatBounds::CityProficiency::VANILLA_MAX;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "▶ 병종 3대 숙련도 편집 (실시간 핫싱크 지원)");

    auto RenderProfEditor = [&](const char* label, const char* idStr, uint16_t& profField) {
        int prof = static_cast<int>(profField);
        const char* tier = "초급";
        if (prof >= 2000) tier = "고급 편성 가능";
        else if (prof >= 1000) tier = "중급 편성 가능";

        ImGui::Text("%s", label);
        ImGui::SameLine(120.0f);
        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::SliderInt(idStr, &prof, 0, maxProf, "%d")) {
            profField = static_cast<uint16_t>(prof);
            valueChanged = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", tier);
    };

    RenderProfEditor("창병 숙련도:", "##SpearSlider", m_record.spear_proficiency);
    RenderProfEditor("기병 숙련도:", "##HorseSlider", m_record.horse_proficiency);
    RenderProfEditor("궁병 숙련도:", "##BowSlider", m_record.bow_proficiency);

    ImGui::Spacing();
    ImGui::Text("빠른 숙련도 치트:");
    ImGui::SameLine();
    if (ImGui::Button("전 병종 숙련도 3,000 완충 (고급 병과 해금)")) {
        QuickCheatMaxProficiencies();
    }
    if (m_limitBreakMode) {
        ImGui::SameLine();
        if (ImGui::Button("한계 돌파 5,000 완충")) {
            m_record.spear_proficiency = 5000;
            m_record.horse_proficiency = 5000;
            m_record.bow_proficiency = 5000;
            valueChanged = true;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.40f, 1.0f), "▶ 성벽 내구도 및 주둔 병력");
    float durRatio = std::clamp(m_record.durability / 5000.0f, 0.0f, 1.0f);
    char durBuf[64];
    std::snprintf(durBuf, sizeof(durBuf), "%u / 5,000", m_record.durability);
    ImGui::Text("성벽 내구도:");
    ImGui::SameLine(120.0f);
    ImGui::ProgressBar(durRatio, ImVec2(260.0f, 0), durBuf);
    ImGui::SameLine();
    ImGui::TextDisabled("(실측 검증 중 / 읽기 전용 보호)");

    ImGui::Spacing();
    int troops = static_cast<int>(m_record.troops);
    int maxTroops = m_limitBreakMode ? Game::StatBounds::CityTroops::LIMIT_BREAK_MAX : Game::StatBounds::CityTroops::VANILLA_MAX;
    ImGui::Text("주둔 병력:");
    ImGui::SameLine(120.0f);
    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::SliderInt("##TroopsSliderMil", &troops, 0, maxTroops, "%d명")) {
        m_record.troops = static_cast<uint32_t>(troops);
        valueChanged = true;
    }

    if (valueChanged && m_autoSync) {
        CommitEditToMemory();
    }
}

void CityDetailModal::RenderTabOfficers() {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f),
                       "▶ 도시 소속 인물 명단 (현역 %zu명, 재야 %zu명, 포로 %zu명)",
                       m_activeOfficers.size(), m_freeOfficers.size(), m_prisoners.size());

    static ImGuiTableFlags offTableFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    auto RenderOfficerTable = [](const char* tableId, const std::vector<Game::CityOfficerEntry>& officers, bool showLoyalty) {
        if (officers.empty()) {
            ImGui::TextDisabled("해당 분류의 인물이 없습니다.");
            return;
        }

        int colCount = showLoyalty ? 8 : 7;
        if (ImGui::BeginTable(tableId, colCount, offTableFlags, ImVec2(0, 220))) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("이름", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("신분/상태", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("통솔", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("무력", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("지력", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("정치", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            if (showLoyalty) {
                ImGui::TableSetupColumn("충성", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            }
            ImGui::TableHeadersRow();

            for (const auto& off : officers) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("#%03u", off.officer_id);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", off.name.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", off.status_desc.c_str());

                auto PrintStat = [](uint8_t stat) {
                    if (stat >= 90) {
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%u", stat);
                    } else if (stat >= 80) {
                        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%u", stat);
                    } else {
                        ImGui::Text("%u", stat);
                    }
                };

                ImGui::TableSetColumnIndex(3);
                PrintStat(off.curr_leadership);

                ImGui::TableSetColumnIndex(4);
                PrintStat(off.curr_war);

                ImGui::TableSetColumnIndex(5);
                PrintStat(off.curr_intel);

                ImGui::TableSetColumnIndex(6);
                PrintStat(off.curr_politics);

                if (showLoyalty) {
                    ImGui::TableSetColumnIndex(7);
                    if (off.loyalty >= 100) {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.35f, 1.0f), "%u", off.loyalty);
                    } else {
                        ImGui::Text("%u", off.loyalty);
                    }
                }
            }
            ImGui::EndTable();
        }
    };

    if (ImGui::CollapsingHeader("소속 현역 무장", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderOfficerTable("ActiveOfficersTable", m_activeOfficers, true);
    }

    if (!m_freeOfficers.empty()) {
        if (ImGui::CollapsingHeader("재야 인물", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderOfficerTable("FreeOfficersTable", m_freeOfficers, false);
        }
    }

    if (!m_prisoners.empty()) {
        if (ImGui::CollapsingHeader("수감 포로", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderOfficerTable("PrisonerOfficersTable", m_prisoners, false);
        }
    }
}

} // namespace San13::UI
