
#include <iostream>
#include <iomanip>
#include <set>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"
#include "game/City.hpp"

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

    // 60개 도시에서 실제 소속 무장이나 군주를 어떻게 가리키는지 확인
    // 도시 구조체:
    // +0x00: vtable
    // +0x04: force_id (세력 ID: 1~10)
    // +0x06: governor (태수 무장 ID)
    // +0x08: 군주 무장 ID?

    for (int c = 0; c < 5; ++c) {
        uintptr_t addr = root_base + CityMemoryLayout::CHUNK_OFFSET + c * CityMemoryLayout::OBJECT_STRIDE;
        uint16_t f = 0, g = 0, lord = 0;
        driver.ReadRaw(addr + 0x04, &f, 2);
        driver.ReadRaw(addr + 0x06, &g, 2);
        driver.ReadRaw(addr + 0x08, &lord, 2);
        std::cout << "도시 " << c << " (" << GetCityName(c) << "): 세력=" << f << " 태수=" << g << " +0x08=" << lord << "\n";
    }

    return 0;
}
