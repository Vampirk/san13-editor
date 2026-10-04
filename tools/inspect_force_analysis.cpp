#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include <iostream>
#include <vector>
#include <iomanip>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "Attach failed\n";
        return 1;
    }
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        return 1;
    }

    std::cout << "RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    // 1. 조조, 동탁, 원소, 유비의 무장 구조체에서 force_id (+0x3C) 확인
    // 조조 ID: 333
    // 무장 청크: RootBase + 0x014BC0, Stride: 0x140
    auto getOfficerForce = [&](uint16_t id) -> uint16_t {
        uintptr_t offAddr = rootBase + 0x014BC0 + (id * 0x140);
        uint16_t fId = 0xFFFF;
        driver.Read<uint16_t>(offAddr + 0x3C, fId);
        return fId;
    };

    std::cout << "\n=== [무장 force_id (+0x3C)] ===\n";
    std::cout << "조조 (333): force_id = " << getOfficerForce(333) << "\n";
    std::cout << "하후돈 (83): force_id = " << getOfficerForce(83) << "\n";
    std::cout << "원소 (18): force_id = " << getOfficerForce(18) << "\n";
    std::cout << "동탁 (480): force_id = " << getOfficerForce(480) << "\n";
    std::cout << "여포 (635): force_id = " << getOfficerForce(635) << "\n";
    std::cout << "유비 (610): force_id = " << getOfficerForce(610) << "\n";
    std::cout << "손견 (355): force_id = " << getOfficerForce(355) << "\n";

    // 2. 도시의 force_id 또는 군주 확인
    // 도시 청크: RootBase + 0x007178, Stride: 0x170, 60개
    // 낙양(ID ?), 진류(ID ?) 등
    std::cout << "\n=== [도시 메모리 전수에서 force_id 후보 탐색] ===\n";
    // 진류 (조조 도시) ID 확인: 도시 0~59
    // 도시 구조체 앞부분 덤프 (0x00 ~ 0x58)
    for (int cid = 0; cid < 5; cid++) {
        uintptr_t cityAddr = rootBase + 0x007178 + (cid * 0x170);
        std::vector<uint8_t> buf(0x58);
        driver.ReadRaw(cityAddr, buf.data(), buf.size());
        std::cout << "City " << cid << ": ";
        for (int b = 0; b < 16; b += 2) {
            uint16_t val = *reinterpret_cast<uint16_t*>(&buf[b]);
            std::cout << "[" << b << "]=" << val << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
