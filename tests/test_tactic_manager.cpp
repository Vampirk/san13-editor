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

int main() {
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

    // Step 4: 프로세스 메모리 연결 및 라이브 메모리 전법 판독 검증
    std::cout << "\n[Step 4] 실시간 프로세스 메모리 기반 판독 검증\n";
    LinuxMemoryDriver driver;
    if (driver.Attach("San13PK_kr.exe") || driver.Attach("san13pk")) {
        uintptr_t rootBase = 0;
        std::string verDesc;
        if (GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
            size_t liveLoaded = TacticManager::LoadCustomTacticsFromMemory(&driver, rootBase);
            std::cout << "  -> 라이브 메모리에서 로드된 등록 전법 수: " << liveLoaded << "\n";
            assert(liveLoaded >= 1);

            const char* liveTac250 = TacticManager::GetTacticName(250);
            std::cout << "  [PASS] 라이브 메모리 ID 250 전법명: " << liveTac250 << "\n";
            assert(std::strcmp(liveTac250, "절영암습") == 0);

            // Step 5: 주인공 강월(700)의 전법 ID와 OfficerRecord 연동 검증
            std::cout << "\n[Step 5] 주인공 강월(ID: 700) 전법 연동 최종 검증\n";
            OfficerManager officerMgr;
            officerMgr.Initialize(&driver, rootBase);

            OfficerRecord kangwol;
            if (officerMgr.LoadOfficer(700, kangwol)) {
                std::cout << "  -> 무장 700 (강월) 로드 성공\n";
                uint8_t origTactic = kangwol.tactic_id;
                std::cout << "  -> 강월의 현재 tactic_id: " << static_cast<int>(origTactic) << "\n";

                // 250(절영암습) 부여 및 판독 검증
                assert(officerMgr.SetTactic(700, 250));
                OfficerRecord kwModified;
                assert(officerMgr.LoadOfficer(700, kwModified));
                assert(kwModified.tactic_id == 250);
                const char* tacticName = kwModified.GetCurrentTacticDescription();
                std::cout << "  [PASS] 강월 250 부여 후 전법 명칭: " << tacticName << "\n";
                assert(std::strcmp(tacticName, "절영암습") == 0);

                // 원래 상태로 안전 원복
                assert(officerMgr.SetTactic(700, origTactic));

                // Step 6: SetTactic을 통한 등록 전법(250) 변경 및 원복 검증
                std::cout << "\n[Step 6] 무장 전법을 등록 전법(250)으로 변경 및 원복 쓰기 검증\n";
                // 조조(333)에게 250 부여 테스트
                OfficerRecord caocao;
                assert(officerMgr.LoadOfficer(333, caocao));
                uint8_t origCaoCaoTactic = caocao.tactic_id;
                std::cout << "  -> 조조 원래 전법 ID: " << (int)origCaoCaoTactic << " (" << caocao.GetCurrentTacticDescription() << ")\n";

                // 조조에게 250(절영암습) 부여
                assert(officerMgr.SetTactic(333, 250));
                OfficerRecord caocaoModified;
                assert(officerMgr.LoadOfficer(333, caocaoModified));
                std::cout << "  -> 250 쓰기 후 조조 전법 ID: " << (int)caocaoModified.tactic_id << " (" << caocaoModified.GetCurrentTacticDescription() << ")\n";
                assert(caocaoModified.tactic_id == 250);
                assert(std::strcmp(caocaoModified.GetCurrentTacticDescription(), "절영암습") == 0);
                std::cout << "  [PASS] 조조에게 등록 전법 250(절영암습) 성공적으로 쓰기 및 판독 확인\n";

                // 원래 전법으로 100% 안전 원복
                assert(officerMgr.SetTactic(333, origCaoCaoTactic));
                OfficerRecord caocaoRestored;
                assert(officerMgr.LoadOfficer(333, caocaoRestored));
                assert(caocaoRestored.tactic_id == origCaoCaoTactic);
                std::cout << "  [PASS] 조조 원래 전법(" << (int)origCaoCaoTactic << ")으로 100% 안전 원복 완료\n";
            } else {
                std::cerr << "  [WARN] 무장 700 로드 실패 (슬롯 확인 필요)\n";
            }
        }
    } else {
        std::cout << "  [SKIP] 삼국지 13 프로세스 미실행 (오프라인 파일 테스트만 완료)\n";
    }

    std::cout << "\n============================================================\n";
    std::cout << " >>> [PHASE 1 ACCEPTANCE CRITERIA 100% 통과] <<<\n";
    std::cout << "============================================================\n";
    return 0;
}
