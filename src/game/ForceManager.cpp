#include "game/ForceManager.hpp"
#include "game/OfficerNameTable.hpp"
#include "core/IMemoryDriver.hpp"

namespace San13::Game {

Core::IMemoryDriver* ForceManager::s_driver = nullptr;
uintptr_t ForceManager::s_forceArrayBase = 0;
std::vector<ForceRecord> ForceManager::s_forces;
bool ForceManager::s_initialized = false;

bool ForceManager::Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase) {
    if (!driver || rootBase == 0) return false;
    s_driver = driver;
    s_forceArrayBase = rootBase + ForceMemoryLayout::CHUNK_OFFSET;
    return Refresh();
}

bool ForceManager::Refresh() {
    if (!s_driver || s_forceArrayBase == 0) return false;

    s_forces.assign(ForceMemoryLayout::MAX_FORCE_COUNT, ForceRecord{});

    for (size_t i = 0; i < ForceMemoryLayout::MAX_FORCE_COUNT; ++i) {
        uintptr_t entryAddr = s_forceArrayBase + (i * ForceMemoryLayout::OBJECT_STRIDE);
        uint16_t rulerId = 0xFFFF;
        if (!s_driver->Read<uint16_t>(entryAddr + ForceMemoryLayout::RULER_OFFSET, rulerId)) {
            continue;
        }

        s_forces[i].force_id = static_cast<uint16_t>(i);
        s_forces[i].ruler_id = rulerId;

        if (rulerId != 0xFFFF && rulerId < 2165) {
            std::string_view rName = GetOfficerKoreanName(rulerId);
            if (!rName.empty()) {
                s_forces[i].ruler_name = std::string(rName);
                s_forces[i].force_name = std::string(rName) + "군";
                s_forces[i].is_valid = true;
            } else {
                s_forces[i].ruler_name = "무장 #" + std::to_string(rulerId);
                s_forces[i].force_name = "세력 " + std::to_string(i);
                s_forces[i].is_valid = true;
            }
        } else {
            s_forces[i].ruler_name.clear();
            s_forces[i].force_name = "무소속";
            s_forces[i].is_valid = false;
        }
    }

    s_initialized = true;
    return true;
}

std::string ForceManager::GetForceName(uint16_t forceId) {
    if (forceId == 0xFFFF || forceId == 255) {
        return "무소속";
    }

    if (!s_initialized) {
        Refresh();
    }

    if (forceId < s_forces.size() && s_forces[forceId].is_valid) {
        return s_forces[forceId].force_name;
    }

    return "세력 " + std::to_string(forceId);
}

uint16_t ForceManager::GetForceRulerId(uint16_t forceId) {
    if (forceId < s_forces.size() && s_forces[forceId].is_valid) {
        return s_forces[forceId].ruler_id;
    }
    return 0xFFFF;
}

bool ForceManager::IsValidForce(uint16_t forceId) {
    if (forceId < s_forces.size()) {
        return s_forces[forceId].is_valid;
    }
    return false;
}

const std::vector<ForceRecord>& ForceManager::GetForces() {
    if (!s_initialized) {
        Refresh();
    }
    return s_forces;
}

} // namespace San13::Game
