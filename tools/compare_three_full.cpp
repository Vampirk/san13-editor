#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
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

    std::vector<uint16_t> ids = {851, 857, 858};
    std::vector<std::string> names = {"범증(재야)", "항적(인게임재야)", "우희(재야)"};

    std::cout << "오프셋 | ";
    for (const auto& n : names) std::cout << std::setw(18) << n << " | ";
    std::cout << "\n" << std::string(75, '-') << "\n";

    for (size_t off = 0x00; off < 0x50; ++off) {
        std::cout << "+0x" << std::hex << std::setw(2) << std::setfill('0') << off << " | ";
        for (uint16_t id : ids) {
            uintptr_t addr = POOL + id * STRIDE;
            uint8_t val = 0;
            driver.ReadRaw(addr + off, &val, 1);
            std::cout << std::dec << std::setw(18) << (int)val << " | ";
        }
        std::cout << "\n";
    }

    return 0;
}
