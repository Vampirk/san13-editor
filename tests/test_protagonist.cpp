#include <iostream>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include "game/Protagonist.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 6.6] Protagonist Domain Unit Test Suite\n";
    std::cout << "============================================================\n";

    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cout << "[-] Warning: Game process not running, skipping live protagonist test.\n";
        return 0;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        std::cerr << "[-] Error: RootBase detection failed!\n";
        return 1;
    }

    OfficerManager mgr;
    if (!mgr.Initialize(&driver, rootBase)) {
        std::cerr << "[-] Error: OfficerManager initialization failed!\n";
        return 1;
    }

    std::cout << "\n[Step 1] Protagonist Detection\n";
    ProtagonistRecord prot;
    bool ok = mgr.GetProtagonistInfo(prot);
    if (!ok || !prot.is_valid) {
        std::cerr << "[-] Failed to detect protagonist!\n";
        return 1;
    }

    std::cout << "  [PASS] Protagonist Detected: " << prot.name << " (ID: " << prot.officer_id << ")\n";
    std::cout << "  [PASS] Personal Gold: " << prot.personal_gold << " 금\n";
    std::cout << "  [PASS] Location: " << prot.city_name << " | Rank: " << prot.rank_desc << "\n";
    std::cout << "  [PASS] Stats: Lead=" << (int)prot.leadership << " War=" << (int)prot.war 
              << " Intel=" << (int)prot.intel << " Pol=" << (int)prot.politics << "\n";

    std::cout << "\n[Step 2] IsProtagonist Query Verification\n";
    if (!mgr.IsProtagonist(prot.officer_id) || mgr.IsProtagonist(9999)) {
        std::cerr << "[-] FAIL: IsProtagonist query mismatch\n";
        return 1;
    }
    std::cout << "  [PASS] IsProtagonist true for ID " << prot.officer_id << "\n";
    std::cout << "  [PASS] IsProtagonist false for invalid ID\n";

    std::cout << "\n[Step 3] Personal Gold Modification & Safe Rollback\n";
    uint32_t originalGold = prot.personal_gold;
    uint32_t testGold = 50000;

    // 수정 → 판독 → (판정 전) 무조건 원복하여 실패 시에도 게임 메모리를 오염시키지 않음
    bool writeOk = mgr.SetProtagonistGold(testGold);
    ProtagonistRecord modProt;
    bool readOk = mgr.GetProtagonistInfo(modProt);

    bool restoreOk = mgr.SetProtagonistGold(originalGold);
    ProtagonistRecord rollProt;
    mgr.GetProtagonistInfo(rollProt);

    if (!writeOk || !readOk || modProt.personal_gold != testGold) {
        std::cerr << "[-] FAIL: Gold modification not reflected (read " << modProt.personal_gold << ")\n";
        return 1;
    }
    std::cout << "  [PASS] Gold modified to " << testGold << " 금\n";

    if (!restoreOk || rollProt.personal_gold != originalGold) {
        std::cerr << "[-] FAIL: Gold rollback failed (read " << rollProt.personal_gold << ", expected " << originalGold << ")\n";
        return 1;
    }
    std::cout << "  [PASS] Gold safely restored to original " << originalGold << " 금 (100% Safe Rollback)\n";

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 주인공 무장 감지, 상태 조회, 소지금 수정 및 복구 100% 통과\n";
    std::cout << "============================================================\n";

    return 0;
}
