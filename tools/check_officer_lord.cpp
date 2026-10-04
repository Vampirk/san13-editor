#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"
#include <iostream>
#include <iomanip>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) return 1;

    uint16_t testIds[] = { 333, 83, 18, 118, 610, 94, 480, 635, 355 };
    const char* names[] = { "조조", "하후돈", "원소", "안량", "유비", "관우", "동탁", "여포", "손견" };

    for (int i = 0; i < 9; i++) {
        uintptr_t addr = rootBase + 0x015EE0 + (testIds[i] * 0x12BC);
        uint16_t off3c = 0, off3e = 0, off3a = 0;
        driver.Read<uint16_t>(addr + 0x3A, off3a);
        driver.Read<uint16_t>(addr + 0x3C, off3c);
        driver.Read<uint16_t>(addr + 0x3E, off3e);
        std::cout << names[i] << " (ID " << testIds[i] << "): "
                  << "+0x3A=" << off3a << ", "
                  << "+0x3C=" << off3c << ", "
                  << "+0x3E=" << off3e << " (이름: " << GetOfficerKoreanName(off3e) << ")\n";
    }
    return 0;
}
