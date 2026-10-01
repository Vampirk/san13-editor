
#include <iostream>
#include <iomanip>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    const uintptr_t POOL = root_base + OfficerMemoryLayout::POOL_OFFSET;
    const size_t STRIDE = OfficerMemoryLayout::OBJECT_STRIDE;

    uintptr_t addr = POOL + 857 * STRIDE;
    uint8_t buf[0x100];
    driver.ReadRaw(addr, buf, 0x100);

    std::cout << "=== [ID 857 항적(항우) +0x00 ~ +0x80 상세 덤프] ===\n";
    for (int i = 0; i < 0x80; i += 16) {
        std::cout << "+0x" << std::hex << std::setw(2) << std::setfill('0') << i << ": ";
        for (int j = 0; j < 16; ++j) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i + j] << " ";
        }
        std::cout << "\n";
    }

    std::cout << "\n특기 블록 (+0x26 ~ +0x39, 14개):\n";
    for (int k = 0x26; k <= 0x39; ++k) {
        std::cout << "+0x" << std::hex << k << ": " << std::dec << (int)buf[k] << "  ";
    }
    std::cout << "\n";

    std::cout << "\n+0x50 블록 (+0x50 ~ +0x5F, 16개 10진수):\n";
    for (int k = 0x50; k <= 0x5F; ++k) {
        std::cout << "+0x" << std::hex << k << "=" << std::dec << (int)buf[k] << " ";
    }
    std::cout << "\n";

    return 0;
}
