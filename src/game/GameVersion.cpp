#include "game/GameVersion.hpp"
#include <iostream>

namespace San13::Game {

static const VersionEntry VERSION_TABLE[] = {
    { 0x59919494, 0x33, 0x01FB94A8, "Korean PK 1.0.0" },
    { 0x5D834FFA, 0x34, 0x01FBC618, "Korean PK 1.0.1" },
    { 0x5E3BA719, 0x35, 0x01FBC5D8, "Korean PK 1.0.2 (Latest)" },
    { 0x582C515D, 0x20, 0x01F984E8, "Japanese PK 1.0.0" },
    { 0x5E3BA678, 0x2A, 0x02DF1288, "Japanese PK Latest" },
    { 0x582C529D, 0x2B, 0x01F93F08, "Traditional Chinese PK 1.0.0" },
    { 0x5E3BA5D5, 0x32, 0x01FCAA18, "Traditional Chinese PK Latest" },
    { 0x58F9BBC5, 0x36, 0x0204CFD0, "English PK 1.0.0" },
    { 0x5E3BA865, 0x39, 0x02052110, "English PK Latest" },
    { 0, 0, 0, nullptr }
};

bool GameVersionDetector::DetectRootBase(Core::IMemoryDriver& driver, uintptr_t& outRootBase, std::string& outDesc) {
    uintptr_t baseAddr = driver.GetModuleBase();
    if (baseAddr == 0) return false;

    // 1. DOS Header: e_lfanew at base + 0x3C
    uint32_t e_lfanew = 0;
    if (!driver.Read<uint32_t>(baseAddr + 0x3C, e_lfanew)) {
        return false;
    }

    // 2. NT Header: TimeDateStamp at base + e_lfanew + 8
    uint32_t timeDateStamp = 0;
    if (!driver.Read<uint32_t>(baseAddr + e_lfanew + 8, timeDateStamp)) {
        return false;
    }

    // 3. Match against known table
    for (int i = 0; VERSION_TABLE[i].description != nullptr; i++) {
        if (VERSION_TABLE[i].timedatestamp == timeDateStamp) {
            outRootBase = baseAddr + VERSION_TABLE[i].global_offset;
            outDesc = VERSION_TABLE[i].description;
            return true;
        }
    }

    return false;
}

} // namespace San13::Game
