
#include <iostream>
#include <iomanip>
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

    // 청크 4: 세력 (Force) 테이블
    // RootBase + 0x00ED60, Stride: 0x148 (328B), 최대 64개
    const uintptr_t FORCE_CHUNK = root_base + 0x00ED60;
    const size_t FORCE_STRIDE = 0x148;

    std::cout << "=== [세력 테이블 스캔 (0 ~ 64)] ===\n";
    for (int i = 0; i < 64; ++i) {
        uintptr_t addr = FORCE_CHUNK + i * FORCE_STRIDE;
        uint16_t force_id = 0;
        uint16_t lord_id = 0;
        uint8_t active = 0;
        driver.ReadRaw(addr + 0x00, &force_id, 2);
        driver.ReadRaw(addr + 0x02, &lord_id, 2);
        driver.ReadRaw(addr + 0x04, &active, 1);

        if (lord_id != 0xFFFF && lord_id != 0) {
            std::cout << "세력 인덱스 " << std::setw(2) << i 
                      << " | ID: " << force_id 
                      << " | 군주무장ID: " << lord_id 
                      << " | 활성: " << (int)active << "\n";
        }
    }

    std::cout << "\n세력 125 (0x7D) 확인:\n";
    uintptr_t addr125 = FORCE_CHUNK + 125 * FORCE_STRIDE;
    uint8_t buf[16] = {0};
    driver.ReadRaw(addr125, buf, 16);
    std::cout << "세력 125 메모리: ";
    for (int k = 0; k < 16; ++k) std::cout << std::hex << (int)buf[k] << " ";
    std::cout << std::dec << "\n";

    return 0;
}
