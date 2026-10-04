#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
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

    // Chunk 0 (0x0000 ~ 0x7178) 덤프
    const size_t CHUNK0_SIZE = 0x7178;
    std::vector<uint8_t> chunk0(CHUNK0_SIZE);
    driver.ReadRaw(rootBase, chunk0.data(), CHUNK0_SIZE);

    // 1. Chunk 0의 시작 부분 (헤더) 덤프
    std::cout << "=== Chunk 0 Header (0x0000 ~ 0x0200) ===\n";
    for (size_t i = 0; i < 0x200; i += 16) {
        std::cout << "+0x" << std::hex << std::setw(4) << std::setfill('0') << i << ": ";
        for (size_t j = 0; j < 16; ++j) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)chunk0[i + j] << " ";
        }
        std::cout << " | ";
        for (size_t j = 0; j < 16; ++j) {
            char c = chunk0[i + j];
            std::cout << (c >= 32 && c <= 126 ? c : '.');
        }
        std::cout << std::dec << "\n";
    }

    // 2. Chunk 0 내의 VTable 포인터들 탐색 (0x70000000 ~ 0x75000000 범위)
    std::cout << "\n=== Chunk 0 Embedded VTables / Pointers ===\n";
    for (size_t i = 0; i < CHUNK0_SIZE - 4; i += 4) {
        uint32_t val = *reinterpret_cast<uint32_t*>(&chunk0[i]);
        if (val >= 0x70000000 && val <= 0x75000000 && (val & 3) == 0) {
            std::cout << "  Offset +0x" << std::hex << i << " -> Pointer: 0x" << val << std::dec << "\n";
        }
    }

    return 0;
}
