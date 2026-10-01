#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdint>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        return 1;
    }

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    // 청크 14: RootBase + 0xC0D260 (Tactics Stride: 0x48 = 72 B)
    const uintptr_t TACTICS_CHUNK = root_base + 0xC0D260;
    const size_t STRIDE = 0x48;

    std::cout << "=== 청크 14 전법 테이블 덤프 (0 ~ 100) ===\n";
    for (size_t id = 0; id < 120; ++id) {
        uintptr_t addr = TACTICS_CHUNK + id * STRIDE;
        std::vector<uint8_t> buf(STRIDE, 0);
        driver.ReadRaw(addr, buf.data(), STRIDE);

        // 앞부분 바이트들 출력
        uint16_t tid = *(uint16_t*)&buf[0];
        uint8_t cost = buf[2];
        std::cout << "ID " << std::setw(3) << id << " | Raw: ";
        for (size_t b = 0; b < 12; ++b) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[b] << " ";
        }
        std::cout << std::dec << "\n";
    }

    return 0;
}
