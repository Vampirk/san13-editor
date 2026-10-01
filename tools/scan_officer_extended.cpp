#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdint>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        return 1;
    }

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    std::vector<std::pair<uint32_t, std::string>> targets = {
        {635, "여포 (Lu Bu)"},
        {94,  "관우 (Guan Yu)"},
        {283, "제갈량 (Zhuge)"},
        {333, "조조 (Cao Cao)"},
        {418, "장비 (Zhang Fei)"}
    };

    const size_t STRIDE = 0x12BC;
    const uintptr_t POOL = root_base + 0x015EE0;

    std::vector<std::vector<uint8_t>> dumps;
    for (auto& t : targets) {
        std::vector<uint8_t> buf(STRIDE, 0);
        driver.ReadRaw(POOL + t.first * STRIDE, buf.data(), STRIDE);
        dumps.push_back(buf);
    }

    std::cout << "\n=== [오프셋 +0x00 ~ +0x100 내 2바이트 정수 스캔] ===\n";
    std::cout << "Offset | 여포(천하무쌍) | 관우(군신) | 제갈량(신산귀모) | 조조(초세지걸) | 장비(만인적)\n";
    std::cout << std::string(85, '-') << "\n";

    for (size_t off = 0; off < 0x100; off += 2) {
        uint16_t v0 = *(uint16_t*)&dumps[0][off];
        uint16_t v1 = *(uint16_t*)&dumps[1][off];
        uint16_t v2 = *(uint16_t*)&dumps[2][off];
        uint16_t v3 = *(uint16_t*)&dumps[3][off];
        uint16_t v4 = *(uint16_t*)&dumps[4][off];

        // 5명 모두 0이 아니고 1 ~ 2000 범위 내인 유력 후보
        if (v0 > 0 && v1 > 0 && v2 > 0 && v3 > 0 && v4 > 0 &&
            v0 < 3000 && v1 < 3000 && v2 < 3000 && v3 < 3000 && v4 < 3000) {
            std::cout << "+0x" << std::hex << std::setw(3) << std::setfill('0') << off << std::dec << "  | "
                      << std::setw(14) << v0 << " | "
                      << std::setw(10) << v1 << " | "
                      << std::setw(16) << v2 << " | "
                      << std::setw(14) << v3 << " | "
                      << std::setw(12) << v4 << "\n";
        }
    }

    return 0;
}
