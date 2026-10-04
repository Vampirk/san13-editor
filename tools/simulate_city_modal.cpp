#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/CityManager.hpp"
#include "game/SettlementManager.hpp"
#include "game/ForceManager.hpp"
#include "game/City.hpp"
#include <iostream>
#include <iomanip>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) return 1;

    CityManager cityMgr;
    cityMgr.Initialize(&driver, rootBase);
    SettlementManager setMgr;
    setMgr.Initialize(&driver, rootBase);
    ForceManager::Initialize(&driver, rootBase);

    // 도시 0 (양평), 1 (북평), 3 (남피), 7 (업), 15 (진류), 18 (낙양) 테스트
    int testCities[] = { 0, 1, 3, 7, 15, 18 };

    for (int cid : testCities) {
        CityRecord city;
        cityMgr.LoadCity(cid, city);
        std::vector<SettlementRecord> settlements;
        setMgr.LoadCitySettlements(cid, city.name, settlements);

        std::cout << "\n============================================================\n";
        std::cout << " 도시 " << cid << " [" << city.name << "] (관할 부락 수: " << settlements.size() << ")\n";
        std::cout << "============================================================\n";
        for (const auto& s : settlements) {
            std::cout << "  - 부락 #" << std::setw(3) << s.id
                      << " [" << std::setw(12) << std::left << s.name << "] " << std::right
                      << " -> 표출 세력: [" << s.force_name << "]"
                      << ", 개발도: " << s.dev_progress << "%\n";
        }
    }
    return 0;
}
