#include "game/CityManager.hpp"
#include "game/Officer.hpp"
#include "game/OfficerNameTable.hpp"
#include <iostream>
#include <cstring>
#include <unordered_set>

namespace San13::Game {

// 삼국지 13 PK 60개 도시 명칭 테이블 (ID 0 ~ 59)
// 출처: San13PK DATA1.S13 Entry 1655 시스템 메시지 테이블 (100% 공식 인게임 순서)
static const char* CITY_NAMES[CityMemoryLayout::MAX_CITY_COUNT] = {
    "양평 (Xiangping)",  "북평 (Beiping)",    "계 (Ji)",           "남피 (Nanpi)",
    "중산 (Zhongshan)",  "거록 (Julu)",       "감릉 (Ganling)",    "업 (Ye)",
    "진양 (Jinyang)",    "상당 (Shangdang)",  "평원 (Pingyuan)",   "북해 (Beihai)",
    "낭야 (Langya)",     "하비 (Xiapi)",      "광릉 (Guangling)",  "제북 (Jibei)",
    "복양 (Puyang)",     "진류 (Chenliu)",    "소패 (Xiaopei)",    "초 (Qiao)",
    "허창 (Xuchang)",    "여남 (Runan)",      "하내 (Henei)",      "낙양 (Luoyang)",
    "홍농 (Hongnong)",   "장안 (Chang'an)",   "안정 (Anding)",     "천수 (Tianshui)",
    "무위 (Wuwei)",      "금성 (Jincheng)",   "무도 (Wudu)",       "한중 (Hanzhong)",
    "영안 (Yong'an)",    "자동 (Zitong)",     "성도 (Chengdu)",    "강주 (Jiangzhou)",
    "장가 (Zhangke)",    "월수 (Yuesui)",     "건녕 (Jianning)",   "영창 (Yongchang)",
    "완 (Wan)",          "신야 (Xinye)",      "양양 (Xiangyang)",  "강하 (Jiangxia)",
    "상용 (Shangyong)",  "강릉 (Jiangling)",  "장사 (Changsha)",   "무릉 (Wuling)",
    "계양 (Guiyang)",    "영릉 (Lingling)",   "수춘 (Shouchun)",   "여강 (Lujiang)",
    "건업 (Jianye)",     "오 (Wu)",           "회계 (Kuaiji)",     "예장 (Yuzhang)",
    "건안 (Jian'an)",    "남해 (Nanhai)",     "합포 (Hapu)",       "교지 (Jiaozhi)"
};

const char* GetCityName(uint8_t id) {
    if (id < CityMemoryLayout::MAX_CITY_COUNT) {
        return CITY_NAMES[id];
    }
    return "알 수 없는 도시";
}

CityManager::CityManager()
    : m_driver(nullptr), m_rootBase(0), m_cityArrayBase(0) {
}

CityManager::~CityManager() {
}

bool CityManager::Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase) {
    if (!driver || rootBase == 0) return false;
    m_driver = driver;
    m_rootBase = rootBase;
    m_cityArrayBase = rootBase + CityMemoryLayout::CHUNK_OFFSET;
    return true;
}

uintptr_t CityManager::GetCityAddress(uint8_t id) const {
    if (m_cityArrayBase == 0) return 0;
    return m_cityArrayBase + (id * CityMemoryLayout::OBJECT_STRIDE);
}

bool CityManager::LoadCity(uint8_t id, CityRecord& outRecord) {
    if (!m_driver || m_cityArrayBase == 0 || id >= CityMemoryLayout::MAX_CITY_COUNT) {
        return false;
    }

    uintptr_t cityAddr = GetCityAddress(id);
    San13CityRaw raw;
    if (!m_driver->Read<San13CityRaw>(cityAddr, raw)) {
        return false;
    }

    outRecord.id = id;
    outRecord.name = GetCityName(id);

    // 도시 지배 군주 및 세력 연동 (+0x3E)
    outRecord.ruler_id = raw.ruler_id;
    if (raw.ruler_id != 0xFFFF && raw.ruler_id < 2165) {
        std::string_view rName = GetOfficerKoreanName(raw.ruler_id);
        if (!rName.empty()) {
            outRecord.ruler_name = std::string(rName);
            outRecord.force_name = std::string(rName) + "군";
        } else {
            outRecord.ruler_name = "무장 #" + std::to_string(raw.ruler_id);
            outRecord.force_name = outRecord.ruler_name + "군";
        }
    } else {
        outRecord.ruler_name = "-";
        outRecord.force_name = "공백지 / 무소속";
    }

    outRecord.gold = raw.gold;
    outRecord.provisions = raw.provisions;
    outRecord.civilian_population = raw.civilian_population;
    outRecord.troops = raw.troops;
    outRecord.total_population = raw.civilian_population + raw.troops;

    // 내구도는 pad_0x102 후보 영역 실측 진행 중 (양평 1000, 낙양 2000)
    uint16_t dur = 0;
    std::memcpy(&dur, &raw.pad_0x102[0], sizeof(uint16_t));
    outRecord.durability = dur;

    // 관할 부락 및 인접 도로망 정보 (확정)
    outRecord.settlement_count = raw.settlement_count;
    for (int i = 0; i < 4; i++) {
        outRecord.connected_city_ids[i] = raw.connected_city_ids[i];
    }

    // 신규 발굴 내정 필드 (읽기 전용)
    outRecord.public_order = raw.public_order;
    outRecord.commerce = raw.commerce;
    outRecord.agriculture = raw.agriculture;
    outRecord.culture = raw.culture;

    // 신규 발굴 병과 숙련도 (읽기 전용)
    outRecord.spear_proficiency = raw.spear_proficiency;
    outRecord.horse_proficiency = raw.horse_proficiency;
    outRecord.bow_proficiency = raw.bow_proficiency;

    // 신규 발굴 소속 인물 통계 (읽기 전용)
    outRecord.officer_count = raw.officer_count;
    outRecord.prisoner_count = raw.prisoner_count;
    outRecord.free_officer_count = raw.free_officer_count;

    outRecord.memory_address = cityAddr;

    return true;
}

bool CityManager::LoadAllCities(std::vector<CityRecord>& outRecords) {
    if (!m_driver || m_cityArrayBase == 0) return false;

    outRecords.clear();
    outRecords.reserve(CityMemoryLayout::MAX_CITY_COUNT);

    for (uint8_t id = 0; id < CityMemoryLayout::MAX_CITY_COUNT; id++) {
        CityRecord rec;
        if (LoadCity(id, rec)) {
            outRecords.push_back(rec);
        }
    }

    return (outRecords.size() == CityMemoryLayout::MAX_CITY_COUNT);
}

uint16_t CityManager::GetCityRulerId(uint8_t cityId) {
    if (!m_driver || m_cityArrayBase == 0 || cityId >= CityMemoryLayout::MAX_CITY_COUNT) {
        return 0xFFFF;
    }
    uintptr_t cityAddr = GetCityAddress(cityId);
    uint16_t rulerId = 0xFFFF;
    if (m_driver->Read<uint16_t>(cityAddr + offsetof(San13CityRaw, ruler_id), rulerId)) {
        return rulerId;
    }
    return 0xFFFF;
}

bool CityManager::LoadCityOfficers(uint8_t cityId, CityOfficerCategory category, std::vector<CityOfficerEntry>& outOfficers) {
    outOfficers.clear();
    if (!m_driver || m_cityArrayBase == 0 || cityId >= CityMemoryLayout::MAX_CITY_COUNT) {
        return false;
    }

    uintptr_t cityAddr = GetCityAddress(cityId);
    San13CityRaw raw;
    if (!m_driver->Read<San13CityRaw>(cityAddr, raw)) {
        return false;
    }

    uint32_t headPtr = 0;
    switch (category) {
        case CityOfficerCategory::Active:
            headPtr = raw.officer_list_ptr;
            break;
        case CityOfficerCategory::Prisoner:
            headPtr = raw.prisoner_list_ptr;
            break;
        case CityOfficerCategory::Free:
            headPtr = raw.free_officer_list_ptr;
            break;
    }

    if (headPtr == 0) {
        return true;
    }

    // Read sentinel node (_Myhead)
    San13CityOfficerNodeRaw sentinel;
    if (!m_driver->Read<San13CityOfficerNodeRaw>(headPtr, sentinel)) {
        return false;
    }

    // Empty list check: sentinel points to itself
    if (sentinel.next_node == headPtr || sentinel.next_node == 0) {
        return true;
    }

    std::unordered_set<uint32_t> visited;
    visited.insert(headPtr);

    uint32_t currAddr = sentinel.next_node;
    while (currAddr != headPtr && currAddr != 0) {
        if (visited.find(currAddr) != visited.end()) {
            // Guard against circular/infinite loops
            break;
        }
        visited.insert(currAddr);

        San13CityOfficerNodeRaw node;
        if (!m_driver->Read<San13CityOfficerNodeRaw>(currAddr, node)) {
            break;
        }

        if (node.officer_obj_ptr != 0) {
            San13OfficerHeader hdr;
            if (m_driver->Read<San13OfficerHeader>(node.officer_obj_ptr, hdr)) {
                CityOfficerEntry entry;
                entry.officer_id = static_cast<uint16_t>(hdr.officer_id);
                entry.officer_addr = node.officer_obj_ptr;
                entry.rank = hdr.rank;
                entry.loyalty = hdr.loyalty;
                entry.force_id = hdr.force_id;
                entry.curr_leadership = hdr.stats.curr_leadership;
                entry.curr_war = hdr.stats.curr_war;
                entry.curr_intel = hdr.stats.curr_intel;
                entry.curr_politics = hdr.stats.curr_politics;

                // Korean name lookup
                std::string_view kName = GetOfficerKoreanName(entry.officer_id);
                if (!kName.empty()) {
                    entry.name = std::string(kName);
                } else {
                    const char* fam = GetFamousOfficerName(entry.officer_id);
                    if (fam) {
                        entry.name = fam;
                    } else {
                        entry.name = "무장 #" + std::to_string(entry.officer_id);
                    }
                }

                // Status description
                if (category == CityOfficerCategory::Prisoner) {
                    entry.status_desc = "포로";
                } else if (category == CityOfficerCategory::Free) {
                    entry.status_desc = "재야";
                } else {
                    if (hdr.rank == 0) {
                        if (hdr.lord_officer_id == entry.officer_id || hdr.lord_officer_id == 0xFFFF) {
                            entry.status_desc = "군주";
                        } else {
                            entry.status_desc = "일반";
                        }
                    } else if (hdr.rank < 10) {
                        static const char* const RANK_DESCS[] = {
                            "군주", "1품관", "2품관", "3품관", "4품관",
                            "5품관", "6품관", "7품관", "8품관", "9품관"
                        };
                        entry.status_desc = RANK_DESCS[hdr.rank];
                    } else {
                        entry.status_desc = "일반";
                    }
                }

                outOfficers.push_back(entry);
            }
        }

        currAddr = node.next_node;
    }

    return true;
}

bool CityManager::LoadAllCityOfficers(uint8_t cityId,
                                      std::vector<CityOfficerEntry>& outActive,
                                      std::vector<CityOfficerEntry>& outPrisoners,
                                      std::vector<CityOfficerEntry>& outFree) {
    bool ok1 = LoadCityOfficers(cityId, CityOfficerCategory::Active, outActive);
    bool ok2 = LoadCityOfficers(cityId, CityOfficerCategory::Prisoner, outPrisoners);
    bool ok3 = LoadCityOfficers(cityId, CityOfficerCategory::Free, outFree);
    return (ok1 && ok2 && ok3);
}

bool CityManager::SetCityGold(uint8_t id, int64_t gold, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint32_t clean = BoundsValidator::SanitizeCitySupplies(gold, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, gold);
    return m_driver->Write<uint32_t>(targetAddr, clean);
}

bool CityManager::SetCityProvisions(uint8_t id, int64_t provisions, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint32_t clean = BoundsValidator::SanitizeCitySupplies(provisions, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, provisions);
    return m_driver->Write<uint32_t>(targetAddr, clean);
}

bool CityManager::SetCityTroops(uint8_t id, int64_t troops, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint32_t clean = BoundsValidator::SanitizeCityTroops(troops, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, troops);
    return m_driver->Write<uint32_t>(targetAddr, clean);
}

bool CityManager::SetCityDurability(uint8_t id, int64_t durability, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    (void)durability;
    (void)policy;
    // Note: +0x058은 자금으로 확정되었으며, 내구도는 +0x102 후보 영역 실측 검증 중입니다.
    // 메모리 오염 방지를 위해 쓰기 작업을 안전하게 가드(보류)합니다.
    return true;
}

bool CityManager::MaximizeCityResources(uint8_t id) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    bool ok1 = SetCityGold(id, StatBounds::CitySupplies::VANILLA_MAX);
    bool ok2 = SetCityProvisions(id, StatBounds::CitySupplies::VANILLA_MAX);
    bool ok3 = SetCityDurability(id, StatBounds::CityDurability::VANILLA_MAX);
    return (ok1 && ok2 && ok3);
}

bool CityManager::SetCityPublicOrder(uint8_t id, int64_t publicOrder, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint16_t clean = BoundsValidator::SanitizeCityPublicOrder(publicOrder, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, public_order);
    return m_driver->Write<uint16_t>(targetAddr, clean);
}

bool CityManager::SetCityCommerce(uint8_t id, int64_t commerce, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint16_t clean = BoundsValidator::SanitizeCityDomestic(commerce, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, commerce);
    return m_driver->Write<uint16_t>(targetAddr, clean);
}

bool CityManager::SetCityAgriculture(uint8_t id, int64_t agriculture, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint16_t clean = BoundsValidator::SanitizeCityDomestic(agriculture, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, agriculture);
    return m_driver->Write<uint16_t>(targetAddr, clean);
}

bool CityManager::SetCityCulture(uint8_t id, int64_t culture, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint16_t clean = BoundsValidator::SanitizeCityDomestic(culture, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, culture);
    return m_driver->Write<uint16_t>(targetAddr, clean);
}

bool CityManager::SetCitySpearProficiency(uint8_t id, int64_t spearProf, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint16_t clean = BoundsValidator::SanitizeCityProficiency(spearProf, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, spear_proficiency);
    return m_driver->Write<uint16_t>(targetAddr, clean);
}

bool CityManager::SetCityHorseProficiency(uint8_t id, int64_t horseProf, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint16_t clean = BoundsValidator::SanitizeCityProficiency(horseProf, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, horse_proficiency);
    return m_driver->Write<uint16_t>(targetAddr, clean);
}

bool CityManager::SetCityBowProficiency(uint8_t id, int64_t bowProf, ClampingPolicy policy) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    uint16_t clean = BoundsValidator::SanitizeCityProficiency(bowProf, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, bow_proficiency);
    return m_driver->Write<uint16_t>(targetAddr, clean);
}

bool CityManager::MaximizeCityDomestic(uint8_t id) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    bool ok1 = SetCityPublicOrder(id, StatBounds::CityPublicOrder::VANILLA_MAX);
    bool ok2 = SetCityCommerce(id, 3000);
    bool ok3 = SetCityAgriculture(id, 3000);
    bool ok4 = SetCityCulture(id, 3000);
    return (ok1 && ok2 && ok3 && ok4);
}

bool CityManager::MaximizeCityProficiency(uint8_t id) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    bool ok1 = SetCitySpearProficiency(id, StatBounds::CityProficiency::VANILLA_MAX);
    bool ok2 = SetCityHorseProficiency(id, StatBounds::CityProficiency::VANILLA_MAX);
    bool ok3 = SetCityBowProficiency(id, StatBounds::CityProficiency::VANILLA_MAX);
    return (ok1 && ok2 && ok3);
}

} // namespace San13::Game
