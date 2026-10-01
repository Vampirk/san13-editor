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
        std::cerr << "[-] 게임 프로세스 연결 실패\n";
        return 1;
    }

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    // 청크 14: 전법 (Tactics)
    // RootBase + 0xC0D260, Stride: 0x48 (72 바이트), 약 200개
    const uintptr_t TACTIC_CHUNK = root_base + 0xC0D260;
    const size_t TACTIC_STRIDE = 0x48;
    const size_t MAX_TACTICS = 200;

    std::cout << "============================================================\n";
    std::cout << " [Step 6.0-2] Chunk 14 전법 테이블 덤프 (RootBase + 0xC0D260)\n";
    std::cout << "============================================================\n";

    for (size_t i = 0; i < 30; ++i) {
        uintptr_t addr = TACTIC_CHUNK + i * TACTIC_STRIDE;
        uint8_t buf[72] = {0};
        if (!driver.ReadRaw(addr, buf, sizeof(buf))) continue;

        // 첫 16바이트 출력
        std::cout << "전법[" << std::setw(3) << i << "] (0x" << std::hex << addr << std::dec << "): ";
        for (int k = 0; k < 16; ++k) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[k] << " ";
        }
        std::cout << std::dec << "\n";
    }

    return 0;
}
