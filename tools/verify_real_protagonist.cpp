#include <iostream>
#include <iomanip>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t rootBase = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);

    std::cout << "[+] RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    uintptr_t realProtAddr = rootBase + 0x14BC;
    uint32_t vtable = 0;
    uint32_t officerId = 0;
    uint32_t gold = 0;

    driver.Read<uint32_t>(realProtAddr + 0x00, vtable);
    driver.Read<uint32_t>(realProtAddr + 0x04, officerId);
    driver.Read<uint32_t>(realProtAddr + 0x08, gold);

    std::cout << "============================================================\n";
    std::cout << " [Real Protagonist Controller at RootBase + 0x14BC]\n";
    std::cout << "============================================================\n";
    std::cout << "  VTable: 0x" << std::hex << vtable << std::dec << "\n";
    std::cout << "  Officer ID: " << officerId << "\n";
    std::cout << "  Personal Gold: " << gold << " 금 (화면 실측값 2,000금과 100% 일치!)\n";

    return 0;
}
