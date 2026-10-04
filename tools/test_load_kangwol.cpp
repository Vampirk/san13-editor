#include <iostream>
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

    OfficerRecord rec;
    if (mgr.LoadOfficer(800, rec)) {
        std::cout << "[+] Officer 800 Loaded Successfully!\n";
        std::cout << "  Name: " << rec.name << "\n";
        std::cout << "  Force ID: " << rec.force_id << "\n";
        std::cout << "  Lord ID: " << rec.lord_officer_id << "\n";
        std::cout << "  City ID: " << rec.city_id << "\n";
        std::cout << "  Rank: " << (int)rec.rank << "\n";
        std::cout << "  Stats: 통=" << (int)rec.stats.curr_leadership 
                  << " 무=" << (int)rec.stats.curr_war 
                  << " 지=" << (int)rec.stats.curr_intel 
                  << " 정=" << (int)rec.stats.curr_politics << "\n";
        std::cout << "  Status Flag: " << (int)rec.status_flag << "\n";
    } else {
        std::cout << "[-] Failed to load officer 800\n";
    }

    return 0;
}
