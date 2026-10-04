#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] Error: Game process not found!\n";
        return 1;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        std::cerr << "[-] Error: RootBase detection failed!\n";
        return 1;
    }

    const uintptr_t OFFICER_BASE = rootBase + 0x015EE0;
    const size_t OFFICER_STRIDE = 0x12BC;

    // 1. 활성 무장 수집
    struct OfficerData {
        uint16_t id;
        std::string name;
        std::vector<uint8_t> buffer;
    };

    std::vector<OfficerData> activeOfficers;

    for (uint16_t id = 0; id < 800; ++id) {
        uintptr_t addr = OFFICER_BASE + id * OFFICER_STRIDE;
        uint32_t vtable = 0;
        driver.Read<uint32_t>(addr, vtable);
        if (vtable != 0x7291c510) continue;

        uint8_t act = 0;
        driver.Read<uint8_t>(addr + 0x4F, act);
        if (act == 1) {
            OfficerData data;
            data.id = id;
            data.name = std::string(GetOfficerKoreanName(id));
            data.buffer.resize(OFFICER_STRIDE);
            driver.ReadRaw(addr, data.buffer.data(), OFFICER_STRIDE);
            activeOfficers.push_back(std::move(data));
        }
    }

    std::cout << "[+] Total Active Officers: " << activeOfficers.size() << "\n";

    // 2. 바이트별로 정확히 1명만 다른 값을 가지는 오프셋 탐색 (Unique non-zero)
    std::cout << "\n=== [Search 1] Offsets where exactly ONE officer has a non-zero value ===\n";
    for (size_t off = 0; off < OFFICER_STRIDE; ++off) {
        int nonZeroCount = 0;
        const OfficerData* candidate = nullptr;

        for (const auto& offData : activeOfficers) {
            if (offData.buffer[off] != 0) {
                nonZeroCount++;
                candidate = &offData;
            }
        }

        if (nonZeroCount == 1) {
            std::cout << "  Offset +0x" << std::hex << off << std::dec 
                      << " (val=" << (int)candidate->buffer[off] << ") -> ONLY on "
                      << candidate->name << " (ID " << candidate->id << ")\n";
        }
    }

    // 3. 비트별로 정확히 1명만 비트가 1인 경우 탐색 (Bitmask)
    std::cout << "\n=== [Search 2] Offsets/Bits where exactly ONE officer has bit set ===\n";
    for (size_t off = 0; off < 0x200; ++off) { // 우선 앞쪽 512바이트 집중
        for (int b = 0; b < 8; ++b) {
            uint8_t mask = (1 << b);
            int bitCount = 0;
            const OfficerData* candidate = nullptr;

            for (const auto& offData : activeOfficers) {
                if ((offData.buffer[off] & mask) != 0) {
                    bitCount++;
                    candidate = &offData;
                }
            }

            if (bitCount == 1) {
                std::cout << "  Offset +0x" << std::hex << off << std::dec 
                          << " bit " << b << " (mask 0x" << std::hex << (int)mask << std::dec 
                          << ") -> ONLY on " << candidate->name << " (ID " << candidate->id << ")\n";
            }
        }
    }

    return 0;
}
