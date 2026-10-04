#include <iostream>
#include <iomanip>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t rootBase = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);

    uintptr_t playerObjAddr = rootBase + 0x9F88;
    std::cout << "[+] RootBase: 0x" << std::hex << rootBase << std::dec << "\n";
    std::cout << "[+] Player Controller Object: 0x" << std::hex << playerObjAddr << std::dec << "\n";

    uint16_t officerId = 0;
    driver.Read<uint16_t>(playerObjAddr + 0x04, officerId);

    uint32_t personalGold = 0;
    driver.Read<uint32_t>(playerObjAddr + 0x48, personalGold);

    std::cout << "============================================================\n";
    std::cout << " [Protagonist Controller Detected]\n";
    std::cout << "============================================================\n";
    std::cout << "  Officer ID: " << officerId << "\n";
    std::cout << "  Name: " << GetOfficerKoreanName(officerId) << "\n";
    std::cout << "  Personal Gold: " << personalGold << " 금\n";

    // 0x00 ~ 0x80 상세 필드 분석
    for (size_t off = 0; off < 0x80; off += 4) {
        uint32_t val = 0;
        driver.Read<uint32_t>(playerObjAddr + off, val);
        if (val != 0) {
            std::cout << "  +0x" << std::hex << std::setw(2) << std::setfill('0') << off 
                      << ": 0x" << val << " (" << std::dec << val << ")\n";
        }
    }

    return 0;
}
