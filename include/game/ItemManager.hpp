#pragma once

#include "core/IMemoryDriver.hpp"
#include "game/Item.hpp"
#include "game/ItemNameTable.hpp"
#include <vector>
#include <cstdint>

namespace San13::Game {

class ItemManager {
public:
    ItemManager() = default;
    ~ItemManager() = default;

    // 아이템 청크 오프셋 (RootBase 상대)
    static constexpr uintptr_t ITEM_CHUNK_OFFSET = 0x9FE8CC;
    static constexpr size_t ITEM_STRIDE = 36;
    static constexpr size_t MAX_ITEM_SLOTS = 400;

    bool Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase);

    // 단일 아이템 조회
    bool LoadItem(uint16_t itemId, ItemRecord& outItem);

    // 전체 유효 아이템 (234개) 조회
    bool LoadAllItems(std::vector<ItemRecord>& outItems);

    // 특정 무장이 소유한 아이템 목록 조회
    bool GetOfficerItems(uint16_t officerId, std::vector<ItemRecord>& outItems);

    // 소유권 변경 (Write)
    bool SetItemOwner(uint16_t itemId, uint16_t newOwnerOfficerId);

    // 편의 함수: 몰수 (0xFFFF = 미소유/시장으로 변경)
    bool ConfiscateItem(uint16_t itemId);

    // 편의 함수: 수여 (지정 무장에게 부여)
    bool GrantItem(uint16_t itemId, uint16_t officerId);

    // 소재지 도시 변경 (미소유 보물 시장 이동)
    bool SetItemLocation(uint16_t itemId, uint8_t cityId);

    uintptr_t GetItemAddress(uint16_t itemId) const;

private:
    Core::IMemoryDriver* m_driver = nullptr;
    uintptr_t m_rootBase = 0;
    uintptr_t m_itemBase = 0;
};

} // namespace San13::Game
