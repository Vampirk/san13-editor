#include "game/ItemManager.hpp"
#include "game/OfficerNameTable.hpp"
#include "game/Officer.hpp"
#include <iostream>
#include <sstream>

namespace San13::Game {

static const char* const SKILL_NAMES[20] = {
    "농업", "상업", "문화", "훈련", "순찰",
    "설파", "교섭", "언변", "덕망", "위풍",
    "신속", "분전", "연전", "공성", "병기",
    "견수", "수련", "일기", "호걸", "귀모"
};

bool ItemManager::Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase) {
    if (!driver || rootBase == 0) {
        return false;
    }
    m_driver = driver;
    m_rootBase = rootBase;
    m_itemBase = rootBase + ITEM_CHUNK_OFFSET;
    return true;
}

uintptr_t ItemManager::GetItemAddress(uint16_t itemId) const {
    if (m_itemBase == 0 || itemId >= MAX_ITEM_SLOTS) {
        return 0;
    }
    return m_itemBase + (itemId * ITEM_STRIDE);
}

bool ItemManager::LoadItem(uint16_t itemId, ItemRecord& outItem) {
    if (!m_driver || m_itemBase == 0 || itemId >= MAX_ITEM_SLOTS) {
        return false;
    }

    uintptr_t addr = GetItemAddress(itemId);
    San13ItemRaw raw{};
    if (!m_driver->ReadRaw(addr, &raw, sizeof(raw))) {
        return false;
    }

    outItem.id = raw.item_id;
    outItem.memory_address = addr;
    outItem.category = raw.category;
    outItem.category_name = std::string(GetItemCategoryName(raw.category));

    // 공식 이름 및 설명
    std::string_view officialName = GetItemOfficialName(itemId);
    if (!officialName.empty()) {
        outItem.name = std::string(officialName);
    } else {
        outItem.name = "명품 #" + std::to_string(itemId);
    }

    std::string_view officialDesc = GetItemOfficialDescription(itemId);
    if (!officialDesc.empty()) {
        outItem.description = std::string(officialDesc);
    } else {
        outItem.description = "";
    }

    outItem.stat_type = raw.stat_type;
    outItem.stat_boost = raw.stat_boost;
    if (raw.stat_boost > 0 && raw.stat_type >= 1 && raw.stat_type <= 4) {
        std::string_view stName = GetItemStatTypeName(raw.stat_type);
        outItem.stat_boost_desc = std::string(stName) + " +" + std::to_string(raw.stat_boost);
    } else if (raw.category == 0 && raw.horse_speed > 0) {
        outItem.stat_boost_desc = "기동 +" + std::to_string(raw.horse_speed);
    } else {
        outItem.stat_boost_desc = "-";
    }

    outItem.horse_speed = raw.horse_speed;
    outItem.skill_id = raw.skill_id;
    outItem.skill_level = raw.skill_level;
    if (raw.skill_id < 20 && raw.skill_level > 0) {
        outItem.skill_desc = std::string(SKILL_NAMES[raw.skill_id]) + " Lv." + std::to_string(raw.skill_level);
    } else {
        outItem.skill_desc = "-";
    }

    outItem.value = raw.value;
    outItem.price = raw.price;

    outItem.owner_id = raw.owner_officer_id;
    outItem.is_owned = (raw.owner_officer_id != 0xFFFF && raw.owner_officer_id <= 2164);
    if (outItem.is_owned) {
        std::string_view on = GetOfficerKoreanName(raw.owner_officer_id);
        if (!on.empty()) {
            outItem.owner_name = std::string(on);
        } else {
            outItem.owner_name = "무장 #" + std::to_string(raw.owner_officer_id);
        }
    } else {
        outItem.owner_name = "미소유/시장";
    }

    return true;
}

bool ItemManager::LoadAllItems(std::vector<ItemRecord>& outItems) {
    outItems.clear();
    outItems.reserve(OFFICIAL_ITEM_COUNT);

    for (uint16_t i = 0; i < OFFICIAL_ITEM_COUNT; i++) {
        ItemRecord rec;
        if (LoadItem(i, rec)) {
            outItems.push_back(std::move(rec));
        }
    }
    return !outItems.empty();
}

bool ItemManager::GetOfficerItems(uint16_t officerId, std::vector<ItemRecord>& outItems) {
    outItems.clear();
    if (!m_driver || m_itemBase == 0) {
        return false;
    }

    for (uint16_t i = 0; i < OFFICIAL_ITEM_COUNT; i++) {
        ItemRecord rec;
        if (LoadItem(i, rec) && rec.owner_id == officerId) {
            outItems.push_back(std::move(rec));
        }
    }
    return true;
}

bool ItemManager::SetItemOwner(uint16_t itemId, uint16_t newOwnerOfficerId) {
    if (!m_driver || m_itemBase == 0 || itemId >= MAX_ITEM_SLOTS) {
        return false;
    }

    // 소유자 ID 유효성: 0xFFFF(미소유) 또는 0~2164
    if (newOwnerOfficerId != 0xFFFF && newOwnerOfficerId > 2164) {
        return false;
    }

    uintptr_t addr = GetItemAddress(itemId);
    uintptr_t ownerOffset = addr + offsetof(San13ItemRaw, owner_officer_id);

    return m_driver->WriteRaw(ownerOffset, &newOwnerOfficerId, sizeof(newOwnerOfficerId));
}

bool ItemManager::ConfiscateItem(uint16_t itemId) {
    return SetItemOwner(itemId, 0xFFFF);
}

bool ItemManager::GrantItem(uint16_t itemId, uint16_t officerId) {
    if (officerId > 2164) return false;
    return SetItemOwner(itemId, officerId);
}

} // namespace San13::Game
