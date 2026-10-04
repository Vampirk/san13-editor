#include <iostream>
#include <iomanip>
#include <vector>
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

    std::cout << "[+] RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    // 1. RootBase + 0x1400 ~ 0x1500 덤프
    std::cout << "=== RootBase + 0x1400 ~ 0x1500 ===\n";
    uint8_t buf[256];
    driver.ReadRaw(rootBase + 0x1400, buf, sizeof(buf));
    for (int i = 0; i < 256; i += 16) {
        std::cout << "+0x" << std::hex << std::setw(4) << std::setfill('0') << (0x1400 + i) << ": ";
        for (int j = 0; j < 16; ++j) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i + j] << " ";
        }
        std::cout << " | ";
        for (int j = 0; j < 16; ++j) {
            char c = buf[i + j];
            std::cout << (c >= 32 && c <= 126 ? c : '.');
        }
        std::cout << std::dec << "\n";
    }

    // 2. 세력 64개에 대해 덤프: 각 세력의 VTable과 군주 ID, 그리고 세력의 제어자(Player vs AI) 플래그가 어디 있는지 확인
    std::cout << "\n=== Forces Analysis (RootBase + 0x00ED60, 328B Stride) ===\n";
    const uintptr_t FORCE_BASE = rootBase + 0x00ED60;
    const size_t FORCE_STRIDE = 0x148;

    for (int f = 0; f < 64; ++f) {
        uintptr_t addr = FORCE_BASE + f * FORCE_STRIDE;
        uint32_t vtable = 0;
        driver.Read<uint32_t>(addr, vtable);
        if (vtable != 0x7291a030) continue; // Force VTable

        // 328바이트 중 처음 64바이트 덤프
        uint8_t fbuf[64] = {0};
        driver.ReadRaw(addr, fbuf, 64);

        uint16_t lordId = *reinterpret_cast<uint16_t*>(&fbuf[4]);
        std::string_view lordName = GetOfficerKoreanName(lordId);

        std::cout << "Force #" << f << " (Lord: " << lordName << " [" << lordId << "]): ";
        for (int i = 0; i < 32; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)fbuf[i] << " ";
        }
        std::cout << std::dec << "\n";
    }

    return 0;
}
