#include "game/CityManager.hpp"
#include <iostream>

namespace San13::Game {

// 삼국지 13 PK 60개 도시 명칭 테이블 (ID 0 ~ 59)
static const char* CITY_NAMES[CityMemoryLayout::MAX_CITY_COUNT] = {
    "양평 (Xiangping)",  "북평 (Beiping)",    "계 (Ji)",           "남피 (Nanpi)",
    "중산 (Zhongshan)",  "거록 (Julu)",       "감릉 (Ganling)",    "안평 (Anping)",
    "진양 (Jinyang)",    "업 (Ye)",           "복양 (Puyang)",     "평원 (Pingyuan)",
    "북해 (Beihai)",     "하비 (Xiapi)",      "소패 (Xiaopei)",    "낭야 (Langya)",
    "광릉 (Guangling)",  "진류 (Chenliu)",    "낙양 (Luoyang)",    "홍농 (Hongnong)",
    "장안 (Chang'an)",   "안정 (Anding)",     "천수 (Tianshui)",   "금성 (Jincheng)",
    "무위 (Wuwei)",      "허창 (Xuchang)",    "초 (Qiao)",         "여남 (Runan)",
    "완 (Wan)",          "신야 (Xinye)",      "양양 (Xiangyang)",  "강하 (Jiangxia)",
    "강릉 (Jiangling)",  "장사 (Changsha)",   "무릉 (Wuling)",     "영릉 (Lingling)",
    "계양 (Guiyang)",    "수춘 (Shouchun)",   "여강 (Lujiang)",    "건업 (Jianye)",
    "오 (Wu)",           "회계 (Kuaiji)",     "시상 (Chaisang)",   "한중 (Hanzhong)",
    "무도 (Wudu)",       "자동 (Zitong)",     "성도 (Chengdu)",    "강주 (Jiangzhou)",
    "영안 (Yong'an)",    "건위 (Jianwei)",    "월수 (Yuesui)",     "운남 (Yunnan)",
    "영창 (Yongchang)",  "건녕 (Jianning)",   "남해 (Nanhai)",     "창오 (Cangwu)",
    "합포 (Hapu)",       "교지 (Jiaozhi)",    "상당 (Shangdang)",  "포판 (Poban)"
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
    outRecord.gold = raw.gold;
    outRecord.provisions = raw.provisions;
    outRecord.troops = raw.troops;
    outRecord.durability = raw.durability;
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
    uint32_t clean = BoundsValidator::SanitizeCityDurability(durability, policy);
    uintptr_t targetAddr = GetCityAddress(id) + offsetof(San13CityRaw, durability);
    return m_driver->Write<uint32_t>(targetAddr, clean);
}

bool CityManager::MaximizeCityResources(uint8_t id) {
    if (!m_driver || id >= CityMemoryLayout::MAX_CITY_COUNT) return false;
    bool ok1 = SetCityGold(id, StatBounds::CitySupplies::VANILLA_MAX);
    bool ok2 = SetCityProvisions(id, StatBounds::CitySupplies::VANILLA_MAX);
    bool ok3 = SetCityDurability(id, StatBounds::CityDurability::VANILLA_MAX);
    return (ok1 && ok2 && ok3);
}

} // namespace San13::Game
