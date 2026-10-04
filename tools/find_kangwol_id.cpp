#include <iostream>
#include <vector>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include "game/OfficerNameTable.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t rootBase = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);

    OfficerManager mgr;
    mgr.Initialize(&driver, rootBase);

    std::vector<OfficerRecord> officers;
    mgr.LoadAllOfficers(officers, true);

    std::cout << "[+] Total loaded officers: " << officers.size() << "\n";
    for (const auto& off : officers) {
        if (off.name.find("강월") != std::string::npos || off.id >= 800) {
            std::cout << "ID " << off.id << ": " << off.name 
                      << " | 통=" << (int)off.stats.curr_leadership
                      << " 무=" << (int)off.stats.curr_war
                      << " 지=" << (int)off.stats.curr_intel
                      << " 정=" << (int)off.stats.curr_politics
                      << " | 세력=" << off.force_id
                      << " | 도시=" << off.city_id
                      << " | 관직=" << (int)off.rank
                      << " | 상태=" << (int)off.status_flag
                      << "\n";
        }
    }

    return 0;
}
