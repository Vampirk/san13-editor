#include <iostream>
#include <iomanip>
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

    uintptr_t objAddr = rootBase + 0x9F88;
    std::cout << "[+] Inspecting Object at RootBase + 0x9F88 (Addr: 0x" << std::hex << objAddr << std::dec << ")\n";

    uint8_t buf[256] = {0};
    driver.ReadRaw(objAddr, buf, sizeof(buf));

    std::cout << "=== 256 Bytes Dump ===\n";
    for (int i = 0; i < 256; i += 16) {
        std::cout << "+0x" << std::hex << std::setw(2) << std::setfill('0') << i << ": ";
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

    uint32_t vtable = *reinterpret_cast<uint32_t*>(&buf[0]);
    uint16_t officerId = *reinterpret_cast<uint16_t*>(&buf[4]);
    std::cout << "\n[+] VTable: 0x" << std::hex << vtable << std::dec << "\n";
    std::cout << "[+] Officer ID: " << officerId << " (" << GetOfficerKoreanName(officerId) << ")\n";

    return 0;
}
