#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <sstream>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"

using namespace San13::Core;
using namespace San13::Game;

struct MemoryRegion {
    uintptr_t start;
    uintptr_t end;
    std::string perms;
    std::string name;
};

std::vector<MemoryRegion> GetProcessRegions(pid_t pid) {
    std::vector<MemoryRegion> regions;
    std::ifstream mapsFile("/proc/" + std::to_string(pid) + "/maps");
    std::string line;
    while (std::getline(mapsFile, line)) {
        std::istringstream iss(line);
        std::string rangeStr, perms, offset, dev, inode, name;
        iss >> rangeStr >> perms >> offset >> dev >> inode;
        std::getline(iss >> std::ws, name);

        size_t dash = rangeStr.find('-');
        if (dash != std::string::npos) {
            uintptr_t start = std::stoull(rangeStr.substr(0, dash), nullptr, 16);
            uintptr_t end = std::stoull(rangeStr.substr(dash + 1), nullptr, 16);
            if (perms.find("rw") != std::string::npos) {
                regions.push_back({start, end, perms, name});
            }
        }
    }
    return regions;
}

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t rootBase = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);

    const uint16_t TARGET_ID = 800; // 강월
    const uintptr_t TARGET_ADDR = rootBase + 0x015EE0 + TARGET_ID * 0x12BC;

    std::cout << "[+] RootBase: 0x" << std::hex << rootBase << std::dec << "\n";
    std::cout << "[+] Target Pointer: 0x" << std::hex << TARGET_ADDR << std::dec << "\n";

    auto regions = GetProcessRegions(driver.GetPid());
    std::cout << "[+] Found " << regions.size() << " writable memory regions\n";

    // 1. TARGET_ADDR (포인터) 검색
    std::cout << "\n=== [Scan 1] Pointer matches (0x" << std::hex << TARGET_ADDR << std::dec << ") ===\n";
    for (const auto& r : regions) {
        size_t size = r.end - r.start;
        if (size > 128 * 1024 * 1024) continue; // 128MB 초과 건너뜀

        std::vector<uint8_t> buf(size);
        if (!driver.ReadRaw(r.start, buf.data(), size)) continue;

        for (size_t i = 0; i <= size - 4; i += 4) {
            uint32_t val = *reinterpret_cast<uint32_t*>(&buf[i]);
            if (val == TARGET_ADDR) {
                std::cout << "  Addr: 0x" << std::hex << (r.start + i) 
                          << " (RootBase + 0x" << (intptr_t)(r.start + i - rootBase) << ")"
                          << " in region: " << r.name << std::dec << "\n";
            }
        }
    }

    return 0;
}
