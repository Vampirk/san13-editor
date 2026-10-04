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

    const uint16_t TARGET_ID = 800; // 강월
    const uintptr_t OFFICER_BASE = rootBase + 0x015EE0;
    const size_t OFFICER_STRIDE = 0x12BC;
    const uintptr_t TARGET_ADDR = OFFICER_BASE + TARGET_ID * OFFICER_STRIDE;

    std::cout << "[+] Target Protagonist: 강월 (ID: " << TARGET_ID 
              << ", Addr: 0x" << std::hex << TARGET_ADDR << std::dec << ")\n";

    // 1. 강월의 구조체 앞부분(128바이트) 덤프
    uint8_t targetBuf[128] = {0};
    driver.ReadRaw(TARGET_ADDR, targetBuf, sizeof(targetBuf));
    std::cout << "\n=== 강월 (ID 800) Header Dump ===\n";
    for (int i = 0; i < 128; i += 16) {
        std::cout << "+0x" << std::hex << std::setw(2) << std::setfill('0') << i << ": ";
        for (int j = 0; j < 16; ++j) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)targetBuf[i + j] << " ";
        }
        std::cout << std::dec << "\n";
    }

    // 2. RootBase + 0x0000 ~ 0x20000 영역에서 TARGET_ID (800 = 0x0320)가 16비트로 저장된 오프셋들 검색
    std::cout << "\n=== Scanning RootBase for uint16_t(800) ===\n";
    for (uintptr_t off = 0; off < 0x20000; off += 2) {
        uint16_t val = 0;
        driver.Read<uint16_t>(rootBase + off, val);
        if (val == TARGET_ID) {
            std::cout << "  [Match uint16] RootBase + 0x" << std::hex << off << std::dec << " == 800\n";
        }
    }

    // 3. RootBase + 0x0000 ~ 0x20000 영역에서 TARGET_ADDR (포인터)가 저장된 오프셋들 검색
    std::cout << "\n=== Scanning RootBase for Pointer(0x" << std::hex << TARGET_ADDR << std::dec << ") ===\n";
    for (uintptr_t off = 0; off < 0x20000; off += 4) {
        uint32_t ptr = 0;
        driver.Read<uint32_t>(rootBase + off, ptr);
        if (ptr == TARGET_ADDR) {
            std::cout << "  [Match Pointer] RootBase + 0x" << std::hex << off << std::dec << " -> 강월 오브젝트\n";
        }
    }

    // 4. 모듈 전체 (.data, .bss 등)에서 TARGET_ADDR 검색
    std::cout << "\n=== Scanning Module Data for Pointer(0x" << std::hex << TARGET_ADDR << std::dec << ") ===\n";
    uintptr_t modBase = driver.GetModuleBase();
    std::cout << "[+] Module Base: 0x" << std::hex << modBase << std::dec << "\n";
    for (uintptr_t off = 0; off < 0x2000000; off += 4) {
        uint32_t ptr = 0;
        driver.Read<uint32_t>(modBase + off, ptr);
        if (ptr == TARGET_ADDR) {
            std::cout << "  [Match Module Pointer] Module + 0x" << std::hex << off 
                      << " (Addr: 0x" << (modBase + off) << ") -> 강월 오브젝트\n";
        }
    }

    return 0;
}
