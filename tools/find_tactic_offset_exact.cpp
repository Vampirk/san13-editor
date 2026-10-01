#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdint>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    const uintptr_t POOL = root_base + OfficerMemoryLayout::POOL_OFFSET;
    const size_t STRIDE = OfficerMemoryLayout::OBJECT_STRIDE;

    // 공식 전법 ID:
    // 0: 천하무쌍 (여포 635)
    // 2: 군신 (관우 94)
    // 3: 만인적 (장비 418)
    // 4: 소패왕 (손책 359)
    struct Target {
        uint16_t id;
        std::string name;
        uint16_t expected_tactic;
    };

    std::vector<Target> targets = {
        {635, "여포", 0}, // 천하무쌍
        {94,  "관우", 2}, // 군신
        {418, "장비", 3}, // 만인적
        {359, "손책", 4}  // 소패왕
    };

    std::vector<std::vector<uint8_t>> dumps;
    for (const auto& t : targets) {
        std::vector<uint8_t> buf(STRIDE, 0);
        driver.ReadRaw(POOL + t.id * STRIDE, buf.data(), STRIDE);
        dumps.push_back(buf);
    }

    std::cout << "============================================================\n";
    std::cout << " [4,796바이트 전수 스캔] 전법 ID (여포=0, 관우=2, 장비=3, 손책=4)\n";
    std::cout << "============================================================\n";

    // 1바이트 (uint8_t) 검사
    std::cout << "\n--- [1바이트 (uint8_t) 후보] ---\n";
    for (size_t off = 0; off < STRIDE; ++off) {
        if (dumps[0][off] == 0 && dumps[1][off] == 2 && dumps[2][off] == 3 && dumps[3][off] == 4) {
            std::cout << "★ [발견!] uint8_t 오프셋 +0x" << std::hex << off << std::dec
                      << " (+0x" << std::hex << off << "): "
                      << "여포=" << (int)dumps[0][off] << ", 관우=" << (int)dumps[1][off]
                      << ", 장비=" << (int)dumps[2][off] << ", 손책=" << (int)dumps[3][off] << "\n";
        }
    }

    // 2바이트 (uint16_t) 검사
    std::cout << "\n--- [2바이트 (uint16_t) 후보] ---\n";
    for (size_t off = 0; off + 1 < STRIDE; ++off) {
        uint16_t v0 = *(uint16_t*)&dumps[0][off];
        uint16_t v1 = *(uint16_t*)&dumps[1][off];
        uint16_t v2 = *(uint16_t*)&dumps[2][off];
        uint16_t v3 = *(uint16_t*)&dumps[3][off];

        if (v0 == 0 && v1 == 2 && v2 == 3 && v3 == 4) {
            std::cout << "★ [발견!] uint16_t 오프셋 +0x" << std::hex << off << std::dec
                      << " (+0x" << std::hex << off << "): "
                      << "여포=" << v0 << ", 관우=" << v1
                      << ", 장비=" << v2 << ", 손책=" << v3 << "\n";
        }
    }

    return 0;
}
