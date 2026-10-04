#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Settlement.hpp"
#include <iostream>
#include <iomanip>
#include <map>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) return 1;

    std::map<uint16_t, int> valFreq;
    uint16_t maxVal = 0, minVal = 0xFFFF;

    for (int i = 0; i < 300; i++) {
        uintptr_t sAddr = rootBase + SettlementMemoryLayout::CHUNK_OFFSET + (i * SettlementMemoryLayout::OBJECT_STRIDE);
        uint16_t val = 0;
        driver.Read<uint16_t>(sAddr + 0x06, val);
        valFreq[val]++;
        if (val > maxVal) maxVal = val;
        if (val < minVal) minVal = val;
    }

    std::cout << "부락 +0x06 최소값: " << minVal << ", 최대값: " << maxVal << "\n";
    std::cout << "분포 (값 : 개수):\n";
    for (auto& p : valFreq) {
        std::cout << p.first << ":" << p.second << "개, ";
    }
    std::cout << "\n";

    return 0;
}
