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

    // 700번 무장 로드
    OfficerRecord rec700;
    if (mgr.LoadOfficer(700, rec700)) {
        std::cout << "=== Officer 700 ===\n";
        std::cout << "  Name: " << rec700.name << "\n";
        std::cout << "  Force ID: " << rec700.force_id << "\n";
        std::cout << "  City ID: " << rec700.city_id << "\n";
        std::cout << "  Rank: " << (int)rec700.rank << "\n";
        std::cout << "  Stats: 통=" << (int)rec700.stats.curr_leadership 
                  << " 무=" << (int)rec700.stats.curr_war 
                  << " 지=" << (int)rec700.stats.curr_intel 
                  << " 정=" << (int)rec700.stats.curr_politics << "\n";
        std::cout << "  Status Flag: " << (int)rec700.status_flag << "\n";
    }

    // 800번 무장 로드
    OfficerRecord rec800;
    if (mgr.LoadOfficer(800, rec800)) {
        std::cout << "=== Officer 800 ===\n";
        std::cout << "  Name: " << rec800.name << "\n";
        std::cout << "  Stats: 통=" << (int)rec800.stats.curr_leadership 
                  << " 무=" << (int)rec800.stats.curr_war 
                  << " 지=" << (int)rec800.stats.curr_intel 
                  << " 정=" << (int)rec800.stats.curr_politics << "\n";
    }

    return 0;
}
