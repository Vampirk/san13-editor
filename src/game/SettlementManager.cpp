#include "game/SettlementManager.hpp"
#include "game/Bounds.hpp"
#include "game/SettlementNameTable.hpp"
#include "game/City.hpp"
#include "game/OfficerNameTable.hpp"
#include "game/ForceManager.hpp"
#include <cstdio>

namespace San13::Game {

SettlementManager::SettlementManager()
    : m_driver(nullptr), m_rootBase(0), m_settlementArrayBase(0) {
}

SettlementManager::~SettlementManager() {
}

bool SettlementManager::Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase) {
    if (!driver || rootBase == 0) return false;
    m_driver = driver;
    m_rootBase = rootBase;
    m_settlementArrayBase = rootBase + SettlementMemoryLayout::CHUNK_OFFSET;
    return true;
}

uintptr_t SettlementManager::GetSettlementAddress(uint16_t id) const {
    if (m_settlementArrayBase == 0) return 0;
    return m_settlementArrayBase + (id * SettlementMemoryLayout::OBJECT_STRIDE);
}

bool SettlementManager::LoadSettlement(uint16_t id, SettlementRecord& outRecord, uint8_t cityId, const std::string& cityName) {
    if (!m_driver || m_settlementArrayBase == 0 || id >= SettlementMemoryLayout::MAX_SETTLEMENT_COUNT) {
        return false;
    }

    uintptr_t addr = GetSettlementAddress(id);
    San13SettlementRaw raw;
    if (!m_driver->Read<San13SettlementRaw>(addr, raw)) {
        return false;
    }

    outRecord.id = raw.settlement_id;
    outRecord.default_effect_id = raw.default_effect_id;
    for (int i = 0; i < 4; i++) {
        outRecord.connected_cities[i] = raw.connected_cities[i];
        outRecord.buff_effects[i] = raw.buff_effects[i];
    }
    outRecord.effect_name = SettlementNameTable::GetEffectDescription(raw.buff_effects);

    uint8_t controllingCity = 255;
    if (cityId != 255) {
        outRecord.city_id = cityId;
        int slot = 0;
        for (int i = 0; i < 4; i++) {
            if (raw.connected_cities[i] == cityId) {
                slot = i;
                break;
            }
        }
        outRecord.dev_progress = raw.city_dev_progress[slot];
        if (outRecord.dev_progress >= 100) {
            controllingCity = cityId;
        }
    } else {
        if (raw.primary_city_id != 0xFF && raw.primary_city_id < CityMemoryLayout::MAX_CITY_COUNT) {
            outRecord.city_id = raw.primary_city_id;
            for (int i = 0; i < 4; i++) {
                if (raw.connected_cities[i] == raw.primary_city_id) {
                    outRecord.dev_progress = raw.city_dev_progress[i];
                    break;
                }
            }
            if (outRecord.dev_progress >= 100) {
                controllingCity = raw.primary_city_id;
            }
        } else {
            outRecord.city_id = raw.connected_cities[0];
            outRecord.dev_progress = raw.city_dev_progress[0];
        }
    }

    // 지배 세력 판정: 완전히 장악(100%)된 도시가 있을 때만 해당 도시 군주의 세력명 부여
    if (controllingCity != 255 && controllingCity < CityMemoryLayout::MAX_CITY_COUNT) {
        uintptr_t cityAddr = m_rootBase + CityMemoryLayout::CHUNK_OFFSET + (controllingCity * CityMemoryLayout::OBJECT_STRIDE);
        uint16_t rulerId = 0xFFFF;
        if (m_driver->Read<uint16_t>(cityAddr + offsetof(San13CityRaw, ruler_id), rulerId) && rulerId != 0xFFFF && rulerId < 2165) {
            std::string_view rName = GetOfficerKoreanName(rulerId);
            if (!rName.empty()) {
                outRecord.force_name = std::string(rName) + "군";
                outRecord.force_id = rulerId;
            } else {
                outRecord.force_name = "무장 #" + std::to_string(rulerId) + "군";
                outRecord.force_id = rulerId;
            }
        } else {
            outRecord.force_name = "공백지 / 무소속";
            outRecord.force_id = 0xFFFF;
        }
    } else {
        outRecord.force_name = "미종속 (중립)";
        outRecord.force_id = 0xFFFF;
    }

    outRecord.name = SettlementNameTable::GetFormattedName(raw.settlement_id, cityName);
    outRecord.memory_address = addr;

    // Phase 4 보완: 요새/성채(건물) 및 요충 레벨(Lv 1~5) 체계
    outRecord.kind = (raw.settlement_id < 147) ? SettlementKind::FixedSettlement : SettlementKind::DynamicFortress;

    // 최대 건물 레벨 산출 (tail_pad[3]에 커스텀 값 있으면 우선 적용)
    uint8_t customMaxLevel = raw.tail_pad[3];
    if (customMaxLevel >= 1 && customMaxLevel <= 5) {
        outRecord.max_fortress_level = customMaxLevel;
    } else {
        outRecord.max_fortress_level = GetDefaultMaxLevel(raw.settlement_id, raw.default_effect_id);
    }

    uint8_t fortTypeVal = raw.tail_pad[0];
    if (fortTypeVal >= 1 && fortTypeVal <= 5) {
        outRecord.fortress_type = static_cast<FortressType>(fortTypeVal);
        outRecord.current_fortress_level = fortTypeVal;
        outRecord.is_fortified = true;
        outRecord.hp = *reinterpret_cast<const uint16_t*>(&raw.tail_pad[1]);
        switch (outRecord.fortress_type) {
            case FortressType::Camp:        outRecord.max_hp = 1000; break; // Lv 1 견진
            case FortressType::MusicTower:  outRecord.max_hp = 1500; break; // Lv 2 군악대
            case FortressType::Hospital:    outRecord.max_hp = 1500; break; // Lv 3 요양사
            case FortressType::SupplyDepot: outRecord.max_hp = 2000; break; // Lv 4 병량고
            case FortressType::Stronghold:  outRecord.max_hp = 2500; break; // Lv 5 방채
            default:                        outRecord.max_hp = 1000; break;
        }
        if (outRecord.hp == 0 || outRecord.hp > outRecord.max_hp) {
            outRecord.hp = outRecord.max_hp;
        }
        outRecord.type_tag = GetFortressTag(outRecord.fortress_type);
    } else {
        outRecord.fortress_type = FortressType::None;
        outRecord.current_fortress_level = 0;
        outRecord.is_fortified = false;
        outRecord.hp = 0;
        outRecord.max_hp = 0;
        if (outRecord.kind == SettlementKind::FixedSettlement) {
            outRecord.type_tag = "[부락]";
        } else {
            outRecord.type_tag = (raw.is_active == 1) ? "[동적 요새]" : "[미사용]";
        }
    }

    return true;
}

bool SettlementManager::LoadAllSettlements(std::vector<SettlementRecord>& outRecords) {
    if (!m_driver || m_settlementArrayBase == 0) return false;

    outRecords.clear();
    outRecords.reserve(SettlementMemoryLayout::MAX_SETTLEMENT_COUNT);

    for (uint16_t id = 0; id < SettlementMemoryLayout::MAX_SETTLEMENT_COUNT; id++) {
        SettlementRecord rec;
        if (LoadSettlement(id, rec)) {
            outRecords.push_back(rec);
        }
    }

    return (outRecords.size() == SettlementMemoryLayout::MAX_SETTLEMENT_COUNT);
}

bool SettlementManager::LoadCitySettlements(uint8_t cityId, const std::string& cityName, std::vector<SettlementRecord>& outRecords) {
    outRecords.clear();
    if (!m_driver || m_settlementArrayBase == 0) return false;

    for (uint16_t id = 0; id < SettlementMemoryLayout::MAX_SETTLEMENT_COUNT; id++) {
        uintptr_t addr = GetSettlementAddress(id);
        San13SettlementRaw raw;
        if (!m_driver->Read<San13SettlementRaw>(addr, raw)) continue;

        bool belongsToCity = false;
        for (int i = 0; i < 4; i++) {
            if (raw.connected_cities[i] == cityId) {
                belongsToCity = true;
                break;
            }
        }

        if (belongsToCity) {
            SettlementRecord rec;
            if (LoadSettlement(id, rec, cityId, cityName)) {
                outRecords.push_back(rec);
            }
        }
    }
    return true;
}

bool SettlementManager::LoadCitySettlements(uint8_t cityId, const uint8_t legacyIds[4], uint32_t count, std::vector<SettlementRecord>& outRecords) {
    (void)legacyIds;
    (void)count;
    return LoadCitySettlements(cityId, "", outRecords);
}

bool SettlementManager::SetSettlementDevProgress(uint16_t id, int64_t devProgress, uint8_t cityId) {
    if (!m_driver || m_settlementArrayBase == 0 || id >= SettlementMemoryLayout::MAX_SETTLEMENT_COUNT) {
        return false;
    }

    uintptr_t addr = GetSettlementAddress(id);
    San13SettlementRaw raw;
    if (!m_driver->Read<San13SettlementRaw>(addr, raw)) {
        return false;
    }

    int slot = 0;
    uint8_t targetCity = (cityId != 255) ? cityId : raw.connected_cities[0];
    if (cityId != 255) {
        for (int i = 0; i < 4; i++) {
            if (raw.connected_cities[i] == cityId) {
                slot = i;
                break;
            }
        }
    }

    uint8_t clean = static_cast<uint8_t>(BoundsValidator::SanitizeSettlementDev(devProgress));
    uintptr_t devAddr = addr + offsetof(San13SettlementRaw, city_dev_progress) + slot;
    if (!m_driver->Write<uint8_t>(devAddr, clean)) {
        return false;
    }

    // [핵심 보완] primary_city_id (+0x13) 동기화
    // 장악도가 100%에 도달하면 반드시 primary_city_id에 대상 도시 ID를 기록해야 인게임 지도에서 지배 부락으로 전환됨!
    uintptr_t primaryCityAddr = addr + offsetof(San13SettlementRaw, primary_city_id);
    if (clean >= 100) {
        if (targetCity != 255) {
            m_driver->Write<uint8_t>(primaryCityAddr, targetCity);
        }
    } else if (clean == 0) {
        // 장악도가 0%로 내려갔고 현재 지배 도시가 해당 도시였다면 중립(0xFF)으로 초기화
        if (raw.primary_city_id == targetCity) {
            m_driver->Write<uint8_t>(primaryCityAddr, 0xFF);
        }
    }

    return true;
}

bool SettlementManager::MaximizeCitySettlements(uint8_t cityId) {
    std::vector<SettlementRecord> settles;
    if (!LoadCitySettlements(cityId, "", settles)) return false;

    bool allOk = true;
    for (const auto& s : settles) {
        if (!SetSettlementDevProgress(s.id, 100, cityId)) {
            allOk = false;
        }
    }
    return allOk;
}

bool SettlementManager::MaximizeCitySettlements(const uint8_t settlementIds[4], uint32_t count) {
    if (!m_driver || !settlementIds) return false;
    uint32_t limit = count > 4 ? 4 : count;
    bool allOk = true;
    for (uint32_t i = 0; i < limit; i++) {
        uint8_t sId = settlementIds[i];
        if (sId == 255 || sId >= SettlementMemoryLayout::MAX_SETTLEMENT_COUNT) continue;
        if (!SetSettlementDevProgress(sId, 100)) {
            allOk = false;
        }
    }
    return allOk;
}

uint8_t SettlementManager::GetDefaultMaxLevel(uint16_t id, uint16_t defaultEffectId) {
    // 1. 삼국지 13 PK 주요 역사적 결전 요충지 (Lv 5 고정)
    if (id == 48 || id == 41 || id == 12 || id == 35 || id == 117 || id == 144) {
        return 5;
    }

    // 2. 특수병과 요충 (호표기 11, 청주병 10, 서량병 13, 동주병 12, 궁기병 9, 등갑병 14, 상병 21)
    if (defaultEffectId >= 9 && defaultEffectId <= 14) return 4;
    if (defaultEffectId == 21) return 4;

    // 3. 대(大) 거점 (상업 2, 농업 5, 병역 8, 사기 17, 지휘 20)
    if (defaultEffectId == 2 || defaultEffectId == 5 || defaultEffectId == 8 ||
        defaultEffectId == 17 || defaultEffectId == 20) {
        return 3;
    }

    // 4. 중(中) 거점 (상업 1, 농업 4, 병역 7, 사기 16, 지휘 19)
    if (defaultEffectId == 1 || defaultEffectId == 4 || defaultEffectId == 7 ||
        defaultEffectId == 16 || defaultEffectId == 19) {
        return 2;
    }

    // 5. 기본 소(小) 거점
    return 1;
}

bool SettlementManager::SetSettlementMaxLevel(uint16_t id, uint8_t maxLevel) {
    if (!m_driver || m_settlementArrayBase == 0 || id >= SettlementMemoryLayout::MAX_SETTLEMENT_COUNT) {
        return false;
    }

    uintptr_t addr = GetSettlementAddress(id);
    uint8_t cleanLv = std::clamp<uint8_t>(maxLevel, 1, 5);
    uintptr_t maxLvAddr = addr + offsetof(San13SettlementRaw, tail_pad) + 3;
    return m_driver->Write<uint8_t>(maxLvAddr, cleanLv);
}

bool SettlementManager::FortifySettlement(uint16_t id, FortressType type, uint16_t hp, uint8_t cityId) {
    if (!m_driver || m_settlementArrayBase == 0 || id >= SettlementMemoryLayout::MAX_SETTLEMENT_COUNT) {
        return false;
    }

    uintptr_t addr = GetSettlementAddress(id);
    San13SettlementRaw raw;
    if (!m_driver->Read<San13SettlementRaw>(addr, raw)) {
        return false;
    }

    if (type == FortressType::None) {
        return DemolishFortress(id);
    }

    // 공식 5대 성채별 기본 최대 내구도 결정
    uint16_t maxHp = 1000;
    uint8_t reqLv = static_cast<uint8_t>(type);
    switch (type) {
        case FortressType::Camp:        maxHp = 1000; reqLv = 1; break; // Lv 1 견진
        case FortressType::MusicTower:  maxHp = 1500; reqLv = 2; break; // Lv 2 군악대
        case FortressType::Hospital:    maxHp = 1500; reqLv = 3; break; // Lv 3 요양사
        case FortressType::SupplyDepot: maxHp = 2000; reqLv = 4; break; // Lv 4 병량고
        case FortressType::Stronghold:  maxHp = 2500; reqLv = 5; break; // Lv 5 방채
        default: break;
    }
    uint16_t setHp = (hp == 0 || hp > maxHp) ? maxHp : hp;

    // 현재 부락의 최대 레벨 확인 및 필요 시 자동 승급
    uint8_t curMaxLv = raw.tail_pad[3];
    if (curMaxLv < reqLv || curMaxLv > 5) {
        curMaxLv = std::max(reqLv, GetDefaultMaxLevel(id, raw.default_effect_id));
    }

    // tail_pad에 타입, HP, 최대 레벨 기록
    uint8_t tail[4] = {
        static_cast<uint8_t>(type),
        static_cast<uint8_t>(setHp & 0xFF),
        static_cast<uint8_t>((setHp >> 8) & 0xFF),
        curMaxLv
    };

    uintptr_t tailAddr = addr + offsetof(San13SettlementRaw, tail_pad);
    if (!m_driver->WriteRaw(tailAddr, tail, sizeof(tail))) {
        return false;
    }

    // 요새화 시 해당 부락을 즉시 100% 장악 상태로 활성화 및 primary_city_id 바인딩
    SetSettlementDevProgress(id, 100, cityId);
    return true;
}

bool SettlementManager::DemolishFortress(uint16_t id) {
    if (!m_driver || m_settlementArrayBase == 0 || id >= SettlementMemoryLayout::MAX_SETTLEMENT_COUNT) {
        return false;
    }

    uintptr_t addr = GetSettlementAddress(id);
    San13SettlementRaw raw;
    if (!m_driver->Read<San13SettlementRaw>(addr, raw)) {
        return false;
    }

    // tail_pad[3]의 최대 레벨 설정은 보존하고 건물 정보(타입/HP)만 해체
    uint8_t maxLv = raw.tail_pad[3];
    uint8_t zeroTail[4] = {0, 0, 0, maxLv};
    uintptr_t tailAddr = addr + offsetof(San13SettlementRaw, tail_pad);
    return m_driver->WriteRaw(tailAddr, zeroTail, sizeof(zeroTail));
}

bool SettlementManager::FortifyAllCitySettlements(uint8_t cityId, FortressType type) {
    std::vector<SettlementRecord> settles;
    if (!LoadCitySettlements(cityId, "", settles)) return false;

    bool allOk = true;
    for (const auto& s : settles) {
        if (!FortifySettlement(s.id, type, 0, cityId)) {
            allOk = false;
        }
    }
    return allOk;
}

const char* SettlementManager::GetFortressTypeName(FortressType type) {
    switch (type) {
        case FortressType::None:        return "일반 부락";
        case FortressType::Camp:        return "견진 (堅陣, Lv1)";
        case FortressType::MusicTower:  return "군악대 (軍樂臺, Lv2)";
        case FortressType::Hospital:    return "요양사 (療養舎, Lv3)";
        case FortressType::SupplyDepot: return "병량고 (兵糧庫, Lv4)";
        case FortressType::Stronghold:  return "방채 (防砦, Lv5)";
        default:                        return "미상";
    }
}

const char* SettlementManager::GetFortressTag(FortressType type) {
    switch (type) {
        case FortressType::None:        return "[부락]";
        case FortressType::Camp:        return "[견진]";
        case FortressType::MusicTower:  return "[군악대]";
        case FortressType::Hospital:    return "[요양사]";
        case FortressType::SupplyDepot: return "[병량고]";
        case FortressType::Stronghold:  return "[방채]";
        default:                        return "[부락]";
    }
}

} // namespace San13::Game
