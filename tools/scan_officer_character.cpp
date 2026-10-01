#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] 게임 프로세스 연결 실패\n";
        return 1;
    }

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    const uintptr_t POOL = root_base + OfficerMemoryLayout::POOL_OFFSET;
    const size_t STRIDE = OfficerMemoryLayout::OBJECT_STRIDE;

    struct OfficerInfo {
        uint16_t id;
        std::string name;
    };

    std::vector<OfficerInfo> officers = {
        {635, "여포"},
        {94,  "관우"},
        {418, "장비"},
        {333, "조조"},
        {283, "제갈량"},
        {610, "유비"},
        {359, "손책"},
        {380, "조운"},
        {857, "항적"},
        {480, "동탁"}
    };

    std::cout << "========================================================================================\n";
    std::cout << " [Phase 6.0 Step 6.0-4] 무장 성격/가치관/기호/인연 심층 오프셋 덤프 (+0x00 ~ +0x80)\n";
    std::cout << "========================================================================================\n\n";

    // 1. +0x25: 전수특기
    std::cout << "--- [+0x25: 전수특기] ---\n";
    for (const auto& off : officers) {
        uint8_t val = 0;
        driver.ReadRaw(POOL + off.id * STRIDE + 0x25, &val, 1);
        std::cout << off.name << "(ID " << off.id << "): " << (int)val 
                  << " (" << OfficerRecord::GetSkillName(val) << ")\n";
    }

    // 2. +0x40 ~ +0x4D: 관계망/가치관 후보 바이트 16진수 및 10진수
    std::cout << "\n--- [+0x40 ~ +0x4F: 16바이트 상세 덤프] ---\n";
    std::cout << "오프셋:     +40 +41 +42 +43 +44 +45 +46 +47 +48 +49 +4A +4B +4C +4D +4E +4F\n";
    std::cout << "설명(추정): [       인연/혈연/가치관/기호 블록       ] [의형] [?] [?] 성격 상태\n";
    for (const auto& off : officers) {
        uint8_t buf[16] = {0};
        driver.ReadRaw(POOL + off.id * STRIDE + 0x40, buf, 16);
        std::cout << std::setw(8) << off.name << ": ";
        for (int i = 0; i < 16; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << "  " << std::dec << std::setfill(' ');
        }
        std::cout << "\n";
    }

    // 3. +0x50 ~ +0x7F: 48바이트 상세 덤프
    std::cout << "\n--- [+0x50 ~ +0x6F: 32바이트 상세 덤프] ---\n";
    std::cout << "오프셋:     +50 +51 +52 +53 +54 +55 +56 +57 +58 +59 +5A +5B +5C +5D +5E +5F\n";
    for (const auto& off : officers) {
        uint8_t buf[16] = {0};
        driver.ReadRaw(POOL + off.id * STRIDE + 0x50, buf, 16);
        std::cout << std::setw(8) << off.name << ": ";
        for (int i = 0; i < 16; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << "  " << std::dec << std::setfill(' ');
        }
        std::cout << "\n";
    }

    std::cout << "\n오프셋:     +60 +61 +62 +63 +64 +65 +66 +67 +68 +69 +6A +6B +6C +6D +6E +6F\n";
    for (const auto& off : officers) {
        uint8_t buf[16] = {0};
        driver.ReadRaw(POOL + off.id * STRIDE + 0x60, buf, 16);
        std::cout << std::setw(8) << off.name << ": ";
        for (int i = 0; i < 16; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << "  " << std::dec << std::setfill(' ');
        }
        std::cout << "\n";
    }

    std::cout << "\n오프셋:     +70 +71 +72 +73 +74 +75 +76 +77 +78 +79 +7A +7B +7C +7D +7E +7F\n";
    for (const auto& off : officers) {
        uint8_t buf[16] = {0};
        driver.ReadRaw(POOL + off.id * STRIDE + 0x70, buf, 16);
        std::cout << std::setw(8) << off.name << ": ";
        for (int i = 0; i < 16; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << "  " << std::dec << std::setfill(' ');
        }
        std::cout << "\n";
    }

    return 0;
}
