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

    std::cout << "[+] RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    // RootBase 근방 (RootBase - 0x100000 ~ RootBase + 0x2000000) 내에서 검색
    uintptr_t scanStart = rootBase > 0x100000 ? rootBase - 0x100000 : 0;
    uintptr_t scanEnd = rootBase + 0x2000000;

    std::cout << "=== Scanning [RootBase - 1MB ~ RootBase + 32MB] for TARGET_ID 800 ===\n";
    size_t chunkSize = 64 * 1024;
    std::vector<uint8_t> buf(chunkSize);

    int count16 = 0, count32 = 0;
    for (uintptr_t addr = scanStart; addr < scanEnd; addr += chunkSize) {
        if (!driver.ReadRaw(addr, buf.data(), chunkSize)) continue;

        for (size_t i = 0; i <= chunkSize - 4; i += 2) {
            uint16_t v16 = *reinterpret_cast<uint16_t*>(&buf[i]);
            uint32_t v32 = *reinterpret_cast<uint32_t*>(&buf[i]);
            uintptr_t curAddr = addr + i;
            intptr_t rOffset = curAddr - rootBase;

            // 만약 800이면서 무장 풀(+0x015EE0) 내부의 officer_id(+0x04)인 경우는 제외하고 관찰
            bool isSelfOfficerId = false;
            if (rOffset >= 0x015EE0 && (rOffset - 0x015EE0) % 0x12BC == 0x04) {
                isSelfOfficerId = true;
            }

            if (v16 == TARGET_ID && !isSelfOfficerId) {
                // RootBase 기준 오프셋이 -0x100000 ~ +0x100000 사이인 흥미로운 주소들 출력
                if (rOffset >= -0x50000 && rOffset <= 0x50000) {
                    std::cout << "  [uint16 Match] Addr: 0x" << std::hex << curAddr 
                              << " (RootBase " << (rOffset >= 0 ? "+0x" : "-0x") << (rOffset >= 0 ? rOffset : -rOffset) << ")"
                              << " [Context: ";
                    // 앞뒤 8바이트 컨텍스트
                    for (int c = -4; c <= 4; ++c) {
                        if (i + c >= 0 && i + c < chunkSize) {
                            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i + c] << " ";
                        }
                    }
                    std::cout << "]\n" << std::dec;
                    count16++;
                }
            }
        }
    }

    std::cout << "[+] Total uint16 matches near RootBase: " << count16 << "\n";

    return 0;
}
