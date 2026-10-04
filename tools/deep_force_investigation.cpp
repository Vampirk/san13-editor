#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) return 1;

    std::cout << "RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    // 1. 세력 청크(RootBase + 0x00ED60, 0x154 Stride, 64개) 정밀 덤프
    std::cout << "\n=== [0x00ED60 청크 슬롯 0~10 덤프 (0x00 ~ 0x40)] ===\n";
    for (int i = 0; i < 10; i++) {
        uintptr_t addr = rootBase + 0x00ED60 + (i * 0x154);
        std::vector<uint8_t> b(0x40);
        driver.ReadRaw(addr, b.data(), b.size());
        std::cout << "Slot " << std::setw(2) << i << ": ";
        for (int j = 0; j < 0x40; j += 2) {
            uint16_t val = *reinterpret_cast<uint16_t*>(&b[j]);
            std::cout << std::hex << std::setw(4) << std::setfill('0') << val << " ";
        }
        std::cout << std::dec << std::setfill(' ') << "\n";
    }

    // 2. 조조 무장 객체(RootBase + 0x015EE0 + 333 * 0x12BC) 정밀 덤프
    uintptr_t caoAddr = rootBase + 0x015EE0 + (333 * 0x12BC);
    std::cout << "\n=== [조조 (ID 333) 헤더 (0x00 ~ 0x60)] ===\n";
    std::vector<uint8_t> cao(0x60);
    driver.ReadRaw(caoAddr, cao.data(), cao.size());
    for (int j = 0; j < 0x60; j += 2) {
        uint16_t val = *reinterpret_cast<uint16_t*>(&cao[j]);
        std::cout << "+0x" << std::hex << std::setw(2) << std::setfill('0') << j << ": " 
                  << std::dec << std::setw(6) << val << " (0x" << std::hex << val << ")\n";
    }

    // 3. 조조의 값 중 25가 나오는 곳, 그리고 0, 1, 2, ... 가 나오는 곳 확인
    // 25라는 숫자가 의미하는 것은? 군단 ID? 세력 ID?
    return 0;
}
