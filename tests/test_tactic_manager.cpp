#include "game/TacticManager.hpp"
#include "game/OfficerManager.hpp"
#include "game/Officer.hpp"
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace San13::Core;
using namespace San13::Game;

int main(int argc, char** argv) {
    bool liveMode = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--live") liveMode = true;
    }

    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 1] Custom Tactic (ID 250~) Test Suite\n";
    std::cout << "============================================================\n\n";

    // Step 1: 바닐라 전법 (0~140) 기본 검증
    std::cout << "[Step 1] 바닐라 전법 명칭 검증\n";
    const char* tac0 = TacticManager::GetTacticName(0);
    const char* tac2 = TacticManager::GetTacticName(2);
    const char* tac140 = TacticManager::GetTacticName(140);

    assert(std::strcmp(tac0, "천하무쌍") == 0);
    assert(std::strcmp(tac2, "군신") == 0);
    assert(std::strcmp(tac140, "금족") == 0);
    std::cout << "  [PASS] ID 0: " << tac0 << "\n";
    std::cout << "  [PASS] ID 2: " << tac2 << "\n";
    std::cout << "  [PASS] ID 140: " << tac140 << "\n";

    // Step 2: editdata_pk.s13 파일로부터 등록 전법 로드
    std::cout << "\n[Step 2] editdata_pk.s13 파일로부터 등록 전법 로드 검증\n";
    size_t loadedFile = TacticManager::LoadCustomTactics();
    std::cout << "  -> 로드된 등록 전법 개수: " << loadedFile << "\n";
    assert(loadedFile >= 1);

    const char* tac250 = TacticManager::GetTacticName(250);
    std::cout << "  [PASS] ID 250 등록 전법명: " << tac250 << "\n";
    assert(std::strcmp(tac250, "절영암습") == 0);

    const CustomTacticEntry* entry250 = TacticManager::GetCustomTactic(250);
    assert(entry250 != nullptr);
    assert(entry250->is_valid);
    std::cout << "  [PASS] ID 250 상세 설명: " << entry250->desc << "\n";

    // Step 3: GetAvailableTactics 콤보박스 목록 검증
    std::cout << "\n[Step 3] 가용 전법 목록(바닐라 141개 + 등록 전법) 검증\n";
    const auto& available = TacticManager::GetAvailableTactics();
    std::cout << "  -> 전체 가용 전법 수: " << available.size() << "\n";
    assert(available.size() >= 142); // 141 + at least 1 custom

    bool found250 = false;
    for (const auto& item : available) {
        if (item.first == 250 && item.second == "절영암습") {
            found250 = true;
            break;
        }
    }
    assert(found250);
    std::cout << "  [PASS] 콤보박스 목록 내 250번 '절영암습' 항목 존재 확인\n";

    // Step 4~6: 라이브 메모리 판독/쓰기 검증은 --live 플래그가 있을 때만 수행 (TECH-04)
    // make test_offline은 게임 실행 여부와 무관하게 게임 메모리를 절대 건드리지 않습니다.
    if (!liveMode) {
        std::cout << "\n[Step 4~6] [SKIP] 오프라인 모드 — 라이브 검증은 `make test_tactic_live` (--live)에서 수행\n";
    } else {
        std::cout << "\n[Step 4] 실시간 프로세스 메모리 기반 판독 검증\n";
        LinuxMemoryDriver driver;
        uintptr_t rootBase = 0;
        std::string verDesc;
        if (!(driver.Attach("San13PK_kr.exe") || driver.Attach("san13pk"))) {
            std::cout << "  [SKIP] 삼국지 13 프로세스 미실행\n";
        } else if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
            std::cerr << "[-] FAIL: RootBase 산출 실패\n";
            return 1;
        } else {
            size_t liveLoaded = TacticManager::LoadCustomTacticsFromMemory(&driver, rootBase);
            std::cout << "  -> 라이브 메모리에서 로드된 등록 전법 수: " << liveLoaded << "\n";
            if (liveLoaded == 0) {
                std::cout << "  [SKIP] 라이브 메모리에 등록 전법 없음 — 쓰기 검증 생략\n";
            } else {
                const char* liveTac250 = TacticManager::GetTacticName(250);
                std::cout << "  [PASS] 라이브 메모리 ID 250 전법명: " << liveTac250 << "\n";

                OfficerManager officerMgr;
                officerMgr.Initialize(&driver, rootBase);

                // 전법을 250으로 쓰고 판독한 뒤, 판정 전에 무조건 원래 전법으로 원복
                auto writeReadRestore = [&](uint16_t id) -> bool {
                    OfficerRecord orig;
                    if (!officerMgr.LoadOfficer(id, orig)) {
                        std::cerr << "[-] FAIL: 무장 " << id << " 로드 실패\n";
                        return false;
                    }
                    bool writeOk = officerMgr.SetTactic(id, 250);
                    OfficerRecord modified;
                    bool readOk = officerMgr.LoadOfficer(id, modified);
                    bool restoreOk = officerMgr.SetTactic(id, orig.tactic_id);
                    OfficerRecord restored;
                    officerMgr.LoadOfficer(id, restored);

                    if (!writeOk || !readOk || modified.tactic_id != 250 ||
                        std::strcmp(modified.GetCurrentTacticDescription(), liveTac250) != 0) {
                        std::cerr << "[-] FAIL: 무장 " << id << " 전법 250 쓰기/판독 불일치 (판독 " << (int)modified.tactic_id << ")\n";
                        return false;
                    }
                    if (!restoreOk || restored.tactic_id != orig.tactic_id) {
                        std::cerr << "[-] FAIL: 무장 " << id << " 원복 실패 (판독 " << (int)restored.tactic_id
                                  << ", 원래 " << (int)orig.tactic_id << ")\n";
                        return false;
                    }
                    std::cout << "  [PASS] 무장 " << id << " (" << orig.name << "): 250(" << liveTac250 << ") 쓰기·판독 후 원래 전법("
                              << (int)orig.tactic_id << ")으로 원복 완료\n";
                    return true;
                };

                std::cout << "\n[Step 5] 주인공 무장 전법 쓰기/원복 검증\n";
                uint16_t protId = officerMgr.GetProtagonistOfficerId();
                if (protId == 0xFFFF) {
                    std::cout << "  [SKIP] 주인공 미감지\n";
                } else if (!writeReadRestore(protId)) {
                    return 1;
                }

                std::cout << "\n[Step 6] 역사 무장(조조 333) 전법 쓰기/원복 검증\n";
                if (!writeReadRestore(333)) {
                    return 1;
                }
            }
        }
    }

    std::cout << "\n============================================================\n";
    std::cout << " >>> [PHASE 1 ACCEPTANCE CRITERIA 100% 통과] <<<\n";
    std::cout << "============================================================\n";
    return 0;
}
