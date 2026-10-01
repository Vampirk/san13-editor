#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
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

    // 베이스 주소: 0x7475b1a4, Stride: 0x80
    const uintptr_t TABLE_BASE = 0x7475b1a4;
    const size_t STRIDE = 0x80;

    struct Target {
        uint16_t id;
        std::string name;
        std::string tactic;
    };

    std::vector<Target> targets = {
        {635, "여포",   "천하무쌍"},
        {94,  "관우",   "군신"},
        {283, "제갈량", "신산귀모"},
        {333, "조조",   "초세지걸"},
        {857, "항적",   "패왕"},
        {380, "조운",   "용담"},
        {418, "장비",   "만인적"}
    };

    std::cout << "============================================================\n";
    std::cout << " [128B 배열 0x7475b1a4] 주요 무장 슬롯 덤프\n";
    std::cout << "============================================================\n";

    for (auto& t : targets) {
        uintptr_t addr = TABLE_BASE + t.id * STRIDE;
        uint8_t buf[128] = {0};
        driver.ReadRaw(addr, buf, sizeof(buf));

        std::cout << "\n[" << t.name << " (ID " << t.id << ") - 전법: " << t.tactic << "]\n";
        for (int row = 0; row < 128; row += 16) {
            std::cout << "+0x" << std::hex << std::setw(2) << std::setfill('0') << row << ": ";
            for (int col = 0; col < 16; ++col) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[row + col] << " ";
            }
            std::cout << std::dec << "\n";
        }
    }

    return 0;
}
