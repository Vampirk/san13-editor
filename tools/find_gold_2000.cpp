#include <iostream>
#include <iomanip>
#include <vector>
#include "core/LinuxMemoryDriver.hpp"
#include "game/Officer.hpp"
#include "game/GameVersion.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t rootBase = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);

    std::cout << "[+] RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    // 1. RootBase + 0x9F88 객체 내부 (0x00 ~ 0x200) 에서 2000(0x07D0) 검색
    std::cout << "\n=== [Search 1] Inside Protagonist Object (RootBase + 0x9F88) ===\n";
    uintptr_t protObj = rootBase + 0x9F88;
    for (size_t off = 0; off < 0x200; off += 2) {
        uint16_t v16 = 0;
        driver.Read<uint16_t>(protObj + off, v16);
        if (v16 == 2000) {
            std::cout << "  Protagonist Object +0x" << std::hex << off << std::dec 
                      << " == 2000 (uint16)\n";
        }
        uint32_t v32 = 0;
        driver.Read<uint32_t>(protObj + off, v32);
        if (v32 == 2000) {
            std::cout << "  Protagonist Object +0x" << std::hex << off << std::dec 
                      << " == 2000 (uint32)\n";
        }
    }

    // 2. 강월의 무장 구조체 (ID 800 or 전체 무장 풀) 내부에서 2000 검색
    std::cout << "\n=== [Search 2] Inside Officer Slots for 2000 ===\n";
    const uintptr_t POOL = rootBase + 0x015EE0;
    const size_t STRIDE = 0x12BC;

    // 강월(ID 800)의 무장 구조체 전체(0x12BC = 4,796B) 스캔
    for (size_t off = 0; off < STRIDE; off += 2) {
        uint16_t v16 = 0;
        driver.Read<uint16_t>(POOL + 800 * STRIDE + off, v16);
        if (v16 == 2000) {
            std::cout << "  Officer 800 +0x" << std::hex << off << std::dec << " == 2000 (uint16)\n";
        }
        uint32_t v32 = 0;
        driver.Read<uint32_t>(POOL + 800 * STRIDE + off, v32);
        if (v32 == 2000) {
            std::cout << "  Officer 800 +0x" << std::hex << off << std::dec << " == 2000 (uint32)\n";
        }
    }

    // 3. 혹시 강월의 ID가 800이 아니라 다른 활성 슬롯(1000+)인가?
    // 모든 2,165 무장 중 +offset 에 2000을 가진 무장 탐색
    std::cout << "\n=== [Search 3] Scanning all 2,165 officers for 2000 ===\n";
    for (uint16_t id = 0; id < 2165; ++id) {
        uintptr_t addr = POOL + id * STRIDE;
        uint32_t vtable = 0;
        driver.Read<uint32_t>(addr, vtable);
        if (vtable != static_cast<uint32_t>(driver.GetModuleBase() + San13::Game::OfficerMemoryLayout::VTABLE_RVA) /* EDGE-56 */) continue;

        // 앞쪽 0x100 바이트 내에 2000이 있는지
        for (size_t off = 0; off < 0x100; off += 2) {
            uint16_t v = 0;
            driver.Read<uint16_t>(addr + off, v);
            if (v == 2000) {
                // 출생/사망년도 등 제외하고
                if (off != 0x0E && off != 0x10 && off != 0x12) {
                    std::cout << "  Officer ID " << id << " +0x" << std::hex << off 
                              << " == 2000 (uint16)\n" << std::dec;
                }
            }
        }
    }

    // 4. RootBase 전체 (0x0000 ~ 0x15EE0) 에서 2000 검색
    std::cout << "\n=== [Search 4] RootBase 0x0000 ~ 0x15EE0 for 2000 ===\n";
    for (uintptr_t off = 0; off < 0x15EE0; off += 4) {
        uint32_t v = 0;
        driver.Read<uint32_t>(rootBase + off, v);
        if (v == 2000) {
            std::cout << "  RootBase +0x" << std::hex << off << " == 2000 (uint32)\n" << std::dec;
        }
    }

    return 0;
}
