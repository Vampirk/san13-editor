#include "game/OfficerManager.hpp"
#include "game/OfficerNameTable.hpp"
#include "game/CityManager.hpp"
#include <iostream>
#include <cstring>

namespace San13::Game {

const char* GetFamousOfficerName(uint16_t id) {
    std::string_view name = GetOfficerKoreanName(id);
    if (!name.empty()) {
        return name.data();
    }
    return nullptr;
}

OfficerManager::OfficerManager()
    : m_driver(nullptr), m_rootBase(0), m_officerArrayBase(0), m_protagonistBase(0) {
}

OfficerManager::~OfficerManager() {
}

bool OfficerManager::Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase) {
    if (!driver || rootBase == 0) return false;
    m_driver = driver;
    m_rootBase = rootBase;
    m_officerArrayBase = rootBase + OfficerMemoryLayout::POOL_OFFSET;
    m_protagonistBase = rootBase + ProtagonistMemoryLayout::CHUNK_OFFSET;
    return true;
}

uintptr_t OfficerManager::GetOfficerAddress(uint16_t id) const {
    if (m_officerArrayBase == 0) return 0;
    return m_officerArrayBase + (id * OfficerMemoryLayout::OBJECT_STRIDE);
}

bool OfficerManager::GetGameDate(uint16_t& outYear, uint16_t& outMonth, uint16_t& outDay) {
    if (!m_driver || m_rootBase == 0) return false;

    // [EDGE-25] RootBase + 0x137A (년), +0x137C (월, 0-indexed: 0=1월..11=12월), +0x1380 (일, 1~31)
    uint16_t y = 0, raw_m = 0, d = 0;
    if (!m_driver->Read<uint16_t>(m_rootBase + 0x137A, y) ||
        !m_driver->Read<uint16_t>(m_rootBase + 0x137C, raw_m) ||
        !m_driver->Read<uint16_t>(m_rootBase + 0x1380, d)) {
        return false;
    }

    if (y < 100 || y > 400) return false;
    if (raw_m > 11) return false;
    if (d < 1 || d > 31) return false;

    outYear = y;
    outMonth = raw_m + 1;
    outDay = d;
    return true;
}

bool OfficerManager::LoadOfficer(uint16_t id, OfficerRecord& outRecord) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }

    uintptr_t objAddr = GetOfficerAddress(id);
    San13OfficerHeader header;
    if (!m_driver->Read<San13OfficerHeader>(objAddr, header)) {
        return false;
    }

    // 확장 슬롯의 경우 VTable 유효성 검사
    if (id >= OfficerMemoryLayout::MAX_HISTORICAL_COUNT && header.vtable != 0x7291c510) {
        return false;
    }

    outRecord.id = id;
    outRecord.force_id = header.force_id;
    outRecord.lord_officer_id = header.lord_officer_id;
    outRecord.face_id = header.face_id;
    outRecord.personality = header.personality;
    outRecord.teach_skill = header.teach_skill;
    std::memcpy(outRecord.skills, header.skills, sizeof(outRecord.skills));
    outRecord.minister_trait_1 = header.minister_trait_1;
    outRecord.minister_trait_2 = header.minister_trait_2;
    outRecord.tactic_id = header.stats.tactic_id;
    outRecord.father_id = header.father_id;
    outRecord.mother_id = header.mother_id;
    outRecord.spouse_1_id = header.spouse_1_id;
    outRecord.spouse_2_id = header.spouse_2_id;
    outRecord.spouse_3_id = header.spouse_3_id;
    outRecord.sworn_brother_id = header.sworn_brother_id;
    outRecord.rival_id = header.rival_id;

    // +0x1284 ~ +0x128E: 친애 무장 6인 슬롯 읽기
    m_driver->ReadRaw(objAddr + OfficerMemoryLayout::FRIENDS_OFFSET, outRecord.friends, sizeof(outRecord.friends));

    // +0x1296 ~ +0x129A: 의형제 3인 슬롯 읽기
    m_driver->ReadRaw(objAddr + OfficerMemoryLayout::SWORN_BROTHERS_OFFSET, outRecord.sworn_brothers, sizeof(outRecord.sworn_brothers));

    // +0x129C ~ +0x12A0: 원수 3인 슬롯 읽기
    m_driver->ReadRaw(objAddr + OfficerMemoryLayout::RIVALS_OFFSET, outRecord.rivals, sizeof(outRecord.rivals));

    // +0x50 ~ +0x5F: 성향/기호/가치관/상성 8대 특성 블록 읽기
    m_driver->Read<San13OfficerCharacter>(objAddr + 0x50, outRecord.character);

    outRecord.city_id = header.city_id;
    outRecord.rank = header.rank;
    outRecord.loyalty = header.loyalty;
    outRecord.birth_year = header.birth_year;
    outRecord.appear_year = header.appear_year;
    outRecord.death_year = header.death_year;
    outRecord.status_flag = header.activity_flag;
    outRecord.is_extended = (id >= OfficerMemoryLayout::MAX_HISTORICAL_COUNT);

    uint16_t curYear = 0, curMonth = 0, curDay = 0;
    if (GetGameDate(curYear, curMonth, curDay)) {
        outRecord.current_year = curYear;
    }

    std::string_view korName = GetOfficerKoreanName(id);
    if (!korName.empty()) {
        outRecord.name = std::string(korName);
    } else if (outRecord.is_extended) {
        outRecord.name = "신무장 / 자녀 #" + std::to_string(id);
    } else {
        outRecord.name = "역사 무장 #" + std::to_string(id);
    }

    outRecord.stats = header.stats;
    outRecord.memory_address = objAddr;

    return true;
}

bool OfficerManager::LoadAllOfficers(std::vector<OfficerRecord>& outRecords, bool includeExtended) {
    if (!m_driver || m_officerArrayBase == 0) return false;

    outRecords.clear();
    outRecords.reserve(includeExtended ? 1300 : OfficerMemoryLayout::MAX_HISTORICAL_COUNT);

    uint16_t curYear = 0, curMonth = 0, curDay = 0;
    bool hasDate = GetGameDate(curYear, curMonth, curDay);

    // 1. 역사 무장 (0 ~ 799)
    for (uint16_t id = 0; id < OfficerMemoryLayout::MAX_HISTORICAL_COUNT; id++) {
        OfficerRecord rec;
        if (LoadOfficer(id, rec)) {
            if (hasDate) rec.current_year = curYear;
            outRecords.push_back(rec);
        }
    }

    // 2. 확장 무장 풀 (800 ~ 2164) - 활성 슬롯 동적 스캔
    if (includeExtended) {
        for (uint16_t id = OfficerMemoryLayout::MAX_HISTORICAL_COUNT; id < OfficerMemoryLayout::MAX_TOTAL_SLOTS; id++) {
            OfficerRecord rec;
            if (LoadOfficer(id, rec)) {
                if (hasDate) rec.current_year = curYear;
                // 실시간 게임에서 생성된 자녀/등록무장 (스탯이 0이 아니거나 활동 중인 슬롯)
                if (rec.stats.curr_leadership > 0 || rec.stats.curr_war > 0 ||
                    rec.stats.curr_intel > 0 || rec.stats.curr_politics > 0) {
                    outRecords.push_back(rec);
                }
            }
        }
    }

    return !outRecords.empty();
}

bool OfficerManager::WriteStats(uint16_t id, const San13OfficerLiveStats& stats, ClampingPolicy policy) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }

    // BoundsValidator를 통한 철저한 방어적 수치 보정
    San13OfficerLiveStats clean = stats;
    clean.base_leadership = BoundsValidator::SanitizeOfficerStat(stats.base_leadership, policy);
    clean.base_war        = BoundsValidator::SanitizeOfficerStat(stats.base_war, policy);
    clean.base_intel      = BoundsValidator::SanitizeOfficerStat(stats.base_intel, policy);
    clean.base_politics   = BoundsValidator::SanitizeOfficerStat(stats.base_politics, policy);

    clean.curr_leadership = BoundsValidator::SanitizeOfficerStat(stats.curr_leadership, policy);
    clean.curr_war        = BoundsValidator::SanitizeOfficerStat(stats.curr_war, policy);
    clean.curr_intel      = BoundsValidator::SanitizeOfficerStat(stats.curr_intel, policy);
    clean.curr_politics   = BoundsValidator::SanitizeOfficerStat(stats.curr_politics, policy);

    clean.exp_leadership  = BoundsValidator::SanitizeExp(stats.exp_leadership);
    clean.exp_war         = BoundsValidator::SanitizeExp(stats.exp_war);
    clean.exp_intel       = BoundsValidator::SanitizeExp(stats.exp_intel);
    clean.exp_politics    = BoundsValidator::SanitizeExp(stats.exp_politics);

    // 적성은 무조건 Strict Guard
    clean.spear_apt       = BoundsValidator::SanitizeAptitude(stats.spear_apt);
    clean.horse_apt       = BoundsValidator::SanitizeAptitude(stats.horse_apt);
    clean.bow_apt         = BoundsValidator::SanitizeAptitude(stats.bow_apt);

    uintptr_t statsAddr = GetOfficerAddress(id) + OfficerMemoryLayout::STATS_OFFSET;
    return m_driver->Write<San13OfficerLiveStats>(statsAddr, clean);
}

bool OfficerManager::SetLeadership(uint16_t id, int64_t val, ClampingPolicy policy) {
    OfficerRecord rec;
    if (!LoadOfficer(id, rec)) return false;
    uint8_t clean = BoundsValidator::SanitizeOfficerStat(val, policy);
    rec.stats.base_leadership = clean;
    rec.stats.curr_leadership = clean;
    return WriteStats(id, rec.stats, policy);
}

bool OfficerManager::SetWar(uint16_t id, int64_t val, ClampingPolicy policy) {
    OfficerRecord rec;
    if (!LoadOfficer(id, rec)) return false;
    uint8_t clean = BoundsValidator::SanitizeOfficerStat(val, policy);
    rec.stats.base_war = clean;
    rec.stats.curr_war = clean;
    return WriteStats(id, rec.stats, policy);
}

bool OfficerManager::SetIntel(uint16_t id, int64_t val, ClampingPolicy policy) {
    OfficerRecord rec;
    if (!LoadOfficer(id, rec)) return false;
    uint8_t clean = BoundsValidator::SanitizeOfficerStat(val, policy);
    rec.stats.base_intel = clean;
    rec.stats.curr_intel = clean;
    return WriteStats(id, rec.stats, policy);
}

bool OfficerManager::SetPolitics(uint16_t id, int64_t val, ClampingPolicy policy) {
    OfficerRecord rec;
    if (!LoadOfficer(id, rec)) return false;
    uint8_t clean = BoundsValidator::SanitizeOfficerStat(val, policy);
    rec.stats.base_politics = clean;
    rec.stats.curr_politics = clean;
    return WriteStats(id, rec.stats, policy);
}

bool OfficerManager::SetAptitudes(uint16_t id, int64_t spear, int64_t horse, int64_t bow) {
    OfficerRecord rec;
    if (!LoadOfficer(id, rec)) return false;
    rec.stats.spear_apt = BoundsValidator::SanitizeAptitude(spear);
    rec.stats.horse_apt = BoundsValidator::SanitizeAptitude(horse);
    rec.stats.bow_apt   = BoundsValidator::SanitizeAptitude(bow);
    return WriteStats(id, rec.stats, ClampingPolicy::SafeVanilla);
}

bool OfficerManager::SetDeathYear(uint16_t id, uint16_t deathYear) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uint16_t cleanYear = deathYear;
    if (cleanYear > StatBounds::OfficerYear::MAX_YEAR) {
        cleanYear = StatBounds::OfficerYear::MAX_YEAR;
    }
    if (cleanYear < StatBounds::OfficerYear::MIN_YEAR) {
        cleanYear = StatBounds::OfficerYear::MIN_YEAR;
    }

    uintptr_t deathAddr = GetOfficerAddress(id) + 0x12;
    return m_driver->Write<uint16_t>(deathAddr, cleanYear);
}

bool OfficerManager::ExtendLife(uint16_t id, int addYears) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uintptr_t deathAddr = GetOfficerAddress(id) + 0x12;
    uint16_t curDeath = 0;
    if (!m_driver->Read<uint16_t>(deathAddr, curDeath)) {
        return false;
    }
    int newDeath = static_cast<int>(curDeath) + addYears;
    return SetDeathYear(id, static_cast<uint16_t>(newDeath));
}

bool OfficerManager::SetTactic(uint16_t id, uint8_t tacticId) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    // 바닐라 0~140 및 등록 전법 250~255 허용 (uint8_t 범위)
    if (tacticId > 140 && tacticId < 250) {
        tacticId = 140;
    }
    uintptr_t tacticAddr = GetOfficerAddress(id) + 0x24;
    return m_driver->Write<uint8_t>(tacticAddr, tacticId);
}

bool OfficerManager::SetFaceId(uint16_t id, uint16_t faceId) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uintptr_t faceAddr = GetOfficerAddress(id) + 0x08;
    return m_driver->Write<uint16_t>(faceAddr, faceId);
}

bool OfficerManager::SetPersonality(uint16_t id, uint8_t personality) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    if (personality > 3) personality = 3;
    uintptr_t persAddr = GetOfficerAddress(id) + 0x4E;
    return m_driver->Write<uint8_t>(persAddr, personality);
}

bool OfficerManager::SetSkills(uint16_t id, const uint8_t skills[20]) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uint8_t cleanSkills[20];
    for (int i = 0; i < 20; ++i) {
        cleanSkills[i] = (skills[i] > 9) ? 9 : skills[i];
    }
    uintptr_t skillsAddr = GetOfficerAddress(id) + 0x26;
    return m_driver->WriteRaw(skillsAddr, cleanSkills, sizeof(cleanSkills));
}

bool OfficerManager::SetCharacter(uint16_t id, const San13OfficerCharacter& character) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uintptr_t charAddr = GetOfficerAddress(id) + 0x50;
    return m_driver->Write<San13OfficerCharacter>(charAddr, character);
}

bool OfficerManager::SetRelations(uint16_t id, uint16_t father, uint16_t mother, uint16_t spouse1, uint16_t swornBrother,
                                  uint16_t spouse2, uint16_t spouse3, uint16_t rival) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uintptr_t addr = GetOfficerAddress(id);
    bool ok = true;
    ok &= m_driver->Write<uint16_t>(addr + 0x40, father);
    ok &= m_driver->Write<uint16_t>(addr + 0x42, mother);
    ok &= m_driver->Write<uint16_t>(addr + 0x44, spouse1);
    ok &= m_driver->Write<uint16_t>(addr + 0x46, spouse2);
    ok &= m_driver->Write<uint16_t>(addr + 0x48, spouse3);
    ok &= m_driver->Write<uint16_t>(addr + 0x4A, swornBrother);
    ok &= m_driver->Write<uint16_t>(addr + 0x4C, rival);
    return ok;
}

bool OfficerManager::SetSwornBrothers(uint16_t id, const uint16_t brothers[3]) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uintptr_t addr = GetOfficerAddress(id);
    bool ok = true;
    ok &= m_driver->WriteRaw(addr + OfficerMemoryLayout::SWORN_BROTHERS_OFFSET, brothers, sizeof(uint16_t) * 3);

    // 헤더 +0x4A 동기화: 유효한 첫 번째 의형제 ID
    uint16_t headerLeader = 0xFFFF;
    for (int i = 0; i < 3; ++i) {
        if (brothers[i] != 0xFFFF && brothers[i] != 0) {
            headerLeader = brothers[i];
            break;
        }
    }
    ok &= m_driver->Write<uint16_t>(addr + 0x4A, headerLeader);
    return ok;
}

bool OfficerManager::SetFriends(uint16_t id, const uint16_t friends[6]) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uintptr_t addr = GetOfficerAddress(id);
    return m_driver->WriteRaw(addr + OfficerMemoryLayout::FRIENDS_OFFSET, friends, sizeof(uint16_t) * 6);
}

bool OfficerManager::SetRivals(uint16_t id, const uint16_t rivals[3]) {
    if (!m_driver || m_officerArrayBase == 0 || id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) {
        return false;
    }
    uintptr_t addr = GetOfficerAddress(id);
    bool ok = true;
    ok &= m_driver->WriteRaw(addr + OfficerMemoryLayout::RIVALS_OFFSET, rivals, sizeof(uint16_t) * 3);

    // 헤더 +0x4C 동기화
    uint16_t headerRival = 0xFFFF;
    for (int i = 0; i < 3; ++i) {
        if (rivals[i] != 0xFFFF && rivals[i] != 0) {
            headerRival = rivals[i];
            break;
        }
    }
    ok &= m_driver->Write<uint16_t>(addr + 0x4C, headerRival);
    return ok;
}

bool OfficerManager::BatchApplyStatsDelta(const std::vector<uint16_t>& ids,
                                          int dLead, int dWar, int dIntel, int dPol,
                                          ClampingPolicy policy) {
    if (!m_driver || m_officerArrayBase == 0) return false;
    bool allOk = true;
    for (uint16_t id : ids) {
        OfficerRecord rec;
        if (!LoadOfficer(id, rec)) {
            allOk = false;
            continue;
        }

        rec.stats.base_leadership = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.base_leadership) + dLead, policy);
        rec.stats.curr_leadership = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.curr_leadership) + dLead, policy);

        rec.stats.base_war = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.base_war) + dWar, policy);
        rec.stats.curr_war = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.curr_war) + dWar, policy);

        rec.stats.base_intel = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.base_intel) + dIntel, policy);
        rec.stats.curr_intel = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.curr_intel) + dIntel, policy);

        rec.stats.base_politics = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.base_politics) + dPol, policy);
        rec.stats.curr_politics = BoundsValidator::SanitizeOfficerStat(static_cast<int>(rec.stats.curr_politics) + dPol, policy);

        if (!WriteStats(id, rec.stats, policy)) {
            allOk = false;
        }
    }
    return allOk;
}

bool OfficerManager::BatchApplyStatsSet(const std::vector<uint16_t>& ids,
                                        int lead, int war, int intel, int pol,
                                        bool setLead, bool setWar, bool setIntel, bool setPol,
                                        ClampingPolicy policy) {
    if (!m_driver || m_officerArrayBase == 0) return false;
    bool allOk = true;
    for (uint16_t id : ids) {
        OfficerRecord rec;
        if (!LoadOfficer(id, rec)) {
            allOk = false;
            continue;
        }

        if (setLead) {
            uint8_t val = BoundsValidator::SanitizeOfficerStat(lead, policy);
            rec.stats.base_leadership = val;
            rec.stats.curr_leadership = val;
        }
        if (setWar) {
            uint8_t val = BoundsValidator::SanitizeOfficerStat(war, policy);
            rec.stats.base_war = val;
            rec.stats.curr_war = val;
        }
        if (setIntel) {
            uint8_t val = BoundsValidator::SanitizeOfficerStat(intel, policy);
            rec.stats.base_intel = val;
            rec.stats.curr_intel = val;
        }
        if (setPol) {
            uint8_t val = BoundsValidator::SanitizeOfficerStat(pol, policy);
            rec.stats.base_politics = val;
            rec.stats.curr_politics = val;
        }

        if (!WriteStats(id, rec.stats, policy)) {
            allOk = false;
        }
    }
    return allOk;
}

bool OfficerManager::BatchApplyAptitudes(const std::vector<uint16_t>& ids, int spear, int horse, int bow) {
    if (!m_driver || m_officerArrayBase == 0) return false;
    bool allOk = true;
    for (uint16_t id : ids) {
        OfficerRecord rec;
        if (!LoadOfficer(id, rec)) {
            allOk = false;
            continue;
        }
        if (spear >= 0) rec.stats.spear_apt = BoundsValidator::SanitizeAptitude(spear);
        if (horse >= 0) rec.stats.horse_apt = BoundsValidator::SanitizeAptitude(horse);
        if (bow >= 0)   rec.stats.bow_apt   = BoundsValidator::SanitizeAptitude(bow);

        if (!WriteStats(id, rec.stats, ClampingPolicy::SafeVanilla)) {
            allOk = false;
        }
    }
    return allOk;
}

bool OfficerManager::BatchExtendLife(const std::vector<uint16_t>& ids, int addYears) {
    if (!m_driver || m_officerArrayBase == 0) return false;
    bool allOk = true;
    for (uint16_t id : ids) {
        if (!ExtendLife(id, addYears)) {
            allOk = false;
        }
    }
    return allOk;
}

bool OfficerManager::CreateSnapshots(const std::vector<uint16_t>& ids, std::vector<OfficerSnapshot>& outSnapshots) {
    if (!m_driver || m_officerArrayBase == 0) return false;
    outSnapshots.clear();
    outSnapshots.reserve(ids.size());
    for (uint16_t id : ids) {
        OfficerRecord rec;
        if (LoadOfficer(id, rec)) {
            OfficerSnapshot snap;
            snap.id = id;
            snap.stats = rec.stats;
            snap.birth_year = rec.birth_year;
            snap.appear_year = rec.appear_year;
            snap.death_year = rec.death_year;
            outSnapshots.push_back(snap);
        }
    }
    return !outSnapshots.empty();
}

bool OfficerManager::BatchRestore(const std::vector<OfficerSnapshot>& snapshots) {
    if (!m_driver || m_officerArrayBase == 0) return false;
    bool allOk = true;
    for (const auto& snap : snapshots) {
        if (!WriteStats(snap.id, snap.stats, ClampingPolicy::LimitBreak)) {
            allOk = false;
        }
        if (!SetDeathYear(snap.id, snap.death_year)) {
            allOk = false;
        }
    }
    return allOk;
}

uint16_t OfficerManager::GetProtagonistOfficerId() {
    if (!m_driver || m_protagonistBase == 0) return 0xFFFF;
    San13ProtagonistRaw raw{};
    if (!m_driver->ReadRaw(m_protagonistBase, &raw, sizeof(raw))) return 0xFFFF;
    if (raw.vtable != ProtagonistMemoryLayout::EXPECTED_VTABLE) return 0xFFFF;
    if (raw.officer_id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) return 0xFFFF;
    return raw.officer_id;
}

bool OfficerManager::IsProtagonist(uint16_t officerId) {
    return (GetProtagonistOfficerId() == officerId);
}

bool OfficerManager::GetProtagonistInfo(ProtagonistRecord& outProtagonist) {
    outProtagonist = ProtagonistRecord{};
    if (!m_driver || m_protagonistBase == 0) return false;

    San13ProtagonistRaw raw{};
    if (!m_driver->ReadRaw(m_protagonistBase, &raw, sizeof(raw))) return false;
    if (raw.vtable != ProtagonistMemoryLayout::EXPECTED_VTABLE) return false;
    if (raw.officer_id >= OfficerMemoryLayout::MAX_TOTAL_SLOTS) return false;

    outProtagonist.is_valid = true;
    outProtagonist.officer_id = raw.officer_id;
    outProtagonist.personal_gold = raw.personal_gold;

    OfficerRecord offRec;
    if (LoadOfficer(raw.officer_id, offRec)) {
        outProtagonist.name = offRec.name;
        outProtagonist.leadership = offRec.stats.curr_leadership;
        outProtagonist.war = offRec.stats.curr_war;
        outProtagonist.intel = offRec.stats.curr_intel;
        outProtagonist.politics = offRec.stats.curr_politics;

        // 관직 / 신분
        if (offRec.rank == 0 && offRec.force_id > 0 && offRec.force_id < 120) {
            outProtagonist.rank_desc = "군주";
        } else if (offRec.rank >= 1 && offRec.rank <= 9) {
            outProtagonist.rank_desc = std::to_string((int)offRec.rank) + "품관";
        } else {
            outProtagonist.rank_desc = "재야";
        }

        // 도시
        if (offRec.city_id < CityMemoryLayout::MAX_CITY_COUNT) {
            outProtagonist.city_name = GetCityName((uint8_t)offRec.city_id);
        } else {
            outProtagonist.city_name = "소재 미상";
        }
    } else {
        outProtagonist.name = "무장 #" + std::to_string(raw.officer_id);
    }

    return true;
}

bool OfficerManager::SetProtagonistGold(uint32_t gold) {
    if (!m_driver || m_protagonistBase == 0) return false;
    uint32_t cleanGold = gold > 999999 ? 999999 : gold;
    return m_driver->Write<uint32_t>(m_protagonistBase + offsetof(San13ProtagonistRaw, personal_gold), cleanGold);
}

} // namespace San13::Game
