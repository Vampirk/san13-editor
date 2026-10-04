#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"
#include "game/City.hpp"
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

    std::cout << "=== 도시 0~59 의 +0x04, +0x06, +0x14, +0x3A, +0x3C, +0x3E, +0x40 덤프 ===\n";
    for (int cid = 0; cid < 60; cid++) {
        uintptr_t cAddr = rootBase + CityMemoryLayout::CHUNK_OFFSET + (cid * CityMemoryLayout::OBJECT_STRIDE);
        uint16_t w04 = 0, w06 = 0, w14 = 0, w3a = 0, w3c = 0, w3e = 0, w40 = 0;
        driver.Read<uint16_t>(cAddr + 0x04, w04);
        driver.Read<uint16_t>(cAddr + 0x06, w06);
        driver.Read<uint16_t>(cAddr + 0x14, w14);
        driver.Read<uint16_t>(cAddr + 0x3A, w3a);
        driver.Read<uint16_t>(cAddr + 0x3C, w3c);
        driver.Read<uint16_t>(cAddr + 0x3E, w3e);
        driver.Read<uint16_t>(cAddr + 0x40, w40);
        const char* name = GetCityName(cid);
        std::cout << "도시 " << std::setw(2) << cid << " [" << std::setw(8) << (name ? name : "") << "]: "
                  << "+0x04=" << std::setw(5) << w04 << " "
                  << "+0x06=" << std::setw(5) << w06 << " "
                  << "+0x14=" << std::setw(5) << w14 << " "
                  << "+0x3A=" << std::setw(5) << w3a << " "
                  << "+0x3C=" << std::setw(5) << w3c << " "
                  << "+0x3E=" << std::setw(5) << w3e << " "
                  << "+0x40=" << std::setw(5) << w40 << " | "
                  << "3E 이름: " << GetOfficerKoreanName(w3e) << ", "
                  << "3C 이름: " << GetOfficerKoreanName(w3c) << "\n";
    }

    return 0;
}
