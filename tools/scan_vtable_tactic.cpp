#include <iostream>
#include <iomanip>
#include <vector>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    // VTable: 0x72919d54 스캔 (RootBase ~ RootBase + 0x1000000)
    const uint32_t TARGET_VTABLE = 0x72919d54;
    const uintptr_t SCAN_START = root_base + 0x800000;
    const uintptr_t SCAN_END   = root_base + 0xF00000;

    std::cout << "=== [VTable 0x72919d54 스캔: 0x" << std::hex << SCAN_START << " ~ 0x" << SCAN_END << "] ===\n";

    std::vector<uintptr_t> matches;
    const size_t CHUNK_SZ = 64 * 1024;
    std::vector<uint8_t> buf(CHUNK_SZ);

    for (uintptr_t addr = SCAN_START; addr < SCAN_END; addr += CHUNK_SZ) {
        if (!driver.ReadRaw(addr, buf.data(), CHUNK_SZ)) continue;
        for (size_t i = 0; i + 4 <= CHUNK_SZ; i += 4) {
            uint32_t val = *(uint32_t*)&buf[i];
            if (val == TARGET_VTABLE) {
                matches.push_back(addr + i);
            }
        }
    }

    std::cout << "총 " << std::dec << matches.size() << "개 오브젝트 발견!\n";
    if (matches.size() >= 2) {
        std::cout << "첫 번째 주소: 0x" << std::hex << matches[0] << "\n";
        std::cout << "두 번째 주소: 0x" << std::hex << matches[1] << "\n";
        std::cout << "오브젝트 간격(Stride): 0x" << (matches[1] - matches[0]) << " (" << std::dec << (matches[1] - matches[0]) << "B)\n";
    }

    // 처음 10개 오브젝트 내용 출력
    for (size_t idx = 0; idx < std::min((size_t)10, matches.size()); ++idx) {
        uintptr_t a = matches[idx];
        uint8_t obj[64] = {0};
        driver.ReadRaw(a, obj, 64);
        std::cout << "\n오브젝트[" << idx << "] (0x" << std::hex << a << "):\n  ";
        for (int k = 0; k < 32; ++k) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)obj[k] << " ";
        }
        std::cout << std::dec << "\n";
    }

    return 0;
}
