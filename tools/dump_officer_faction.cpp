#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include <iostream>
#include <vector>
#include <iomanip>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) return 1;

    // 조조(333), 원소(18), 유비(610), 동탁(480), 손견(355) 무장 헤더 (0x00 ~ 0x60) 16비트/8비트 단위 덤프
    uint16_t testIds[] = { 333, 83, 18, 118, 610, 94, 480, 635, 355 };
    const char* names[] = { "조조", "하후돈", "원소", "안량", "유비", "관우", "동탁", "여포", "손견" };

    std::cout << "Offset | ";
    for (int i = 0; i < 9; i++) {
        std::cout << std::setw(8) << names[i] << " | ";
    }
    std::cout << "\n-------------------------------------------------------------------------------------------------------\n";

    for (int off = 0; off < 0x60; off += 2) {
        std::cout << " +0x" << std::hex << std::setw(2) << std::setfill('0') << off << std::dec << std::setfill(' ') << " | ";
        for (int i = 0; i < 9; i++) {
            uintptr_t addr = rootBase + 0x014BC0 + (testIds[i] * 0x140) + off;
            uint16_t val = 0;
            driver.Read<uint16_t>(addr, val);
            std::cout << std::setw(8) << val << " | ";
        }
        std::cout << "\n";
    }

    return 0;
}
