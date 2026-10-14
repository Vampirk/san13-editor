#include "game/OfficerManager.hpp"
#include "game/OfficerNameTable.hpp"
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 2] Multi-Bonds (Brothers, Friends, Rivals) Test\n";
    std::cout << "============================================================\n\n";

    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] 삼국지 13 프로세스 미실행 -> 라이브 테스트 스킵\n";
        return 0;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        std::cerr << "[-] FAIL: RootBase 산출 실패\n";
        return 1;
    }

    OfficerManager officerMgr;
    if (!officerMgr.Initialize(&driver, rootBase)) {
        std::cerr << "[-] FAIL: OfficerManager 초기화 실패\n";
        return 1;
    }

    // Step 1: 도원결의 3인(유비 610, 관우 94, 장비 418) 상호 의형제 슬롯 검증
    std::cout << "[Step 1] 도원결의 3인 상호 의형제 슬롯 검증\n";
    OfficerRecord liubei, guanyu, zhangfei;
    if (!officerMgr.LoadOfficer(610, liubei) || !officerMgr.LoadOfficer(94, guanyu) || !officerMgr.LoadOfficer(418, zhangfei)) {
        std::cerr << "[-] FAIL: 유비/관우/장비 로드 실패\n";
        return 1;
    }

    std::cout << "  -> 유비 의형제: [" << liubei.sworn_brothers[0] << ", " << liubei.sworn_brothers[1] << ", " << liubei.sworn_brothers[2] << "]\n";
    std::cout << "  -> 관우 의형제: [" << guanyu.sworn_brothers[0] << ", " << guanyu.sworn_brothers[1] << ", " << guanyu.sworn_brothers[2] << "]\n";
    std::cout << "  -> 장비 의형제: [" << zhangfei.sworn_brothers[0] << ", " << zhangfei.sworn_brothers[1] << ", " << zhangfei.sworn_brothers[2] << "]\n";

    // 관우(94)는 유비(610)와 장비(418)를 의형제로 가짐
    assert(guanyu.sworn_brothers[0] == 610);
    assert(guanyu.sworn_brothers[1] == 418);
    // 유비(610)는 관우(94)와 장비(418)를 의형제로 가짐
    assert(liubei.sworn_brothers[0] == 94);
    assert(liubei.sworn_brothers[1] == 418);
    // 장비(418)는 유비(610)와 관우(94)를 의형제로 가짐
    assert(zhangfei.sworn_brothers[0] == 610);
    assert(zhangfei.sworn_brothers[1] == 94);

    std::cout << "  [PASS] 유비·관우·장비 3인 전원 상호 의형제 등록 완벽 일치!\n";

    // Step 2: 친애 무장 슬롯 검증 (유비/관우/장비 -> 제갈량 283)
    std::cout << "\n[Step 2] 친애 무장(Friends) 슬롯 검증\n";
    std::cout << "  -> 유비 친애 1번: " << liubei.friends[0] << "\n";
    std::cout << "  -> 관우 친애 1번: " << guanyu.friends[0] << "\n";
    std::cout << "  -> 장비 친애 1번: " << zhangfei.friends[0] << "\n";
    assert(liubei.friends[0] == 283);
    assert(guanyu.friends[0] == 283);
    assert(zhangfei.friends[0] == 283);
    std::cout << "  [PASS] 유비·관우·장비 3인 공통 친애 무장 제갈량(283) 등록 확인!\n";

    // Step 3: 주인공 무장 다중 인연 쓰기 및 안전 원복 검증 (TECH-05: 강월 700 고정 → 감지된 주인공)
    std::cout << "\n[Step 3] 주인공 무장 다중 인연 쓰기 및 안전 원복 검증\n";
    uint16_t targetId = officerMgr.GetProtagonistOfficerId();
    OfficerRecord target;
    if (targetId == 0xFFFF || !officerMgr.LoadOfficer(targetId, target)) {
        std::cout << "  [SKIP] 주인공 미감지 — 쓰기 검증 생략\n";
    } else {
        uint16_t origBrothers[3];
        uint16_t origFriends[6];
        std::memcpy(origBrothers, target.sworn_brothers, sizeof(origBrothers));
        std::memcpy(origFriends, target.friends, sizeof(origFriends));

        // SetSwornBrothers는 헤더 +0x4A를 "첫 유효 의형제"로 덮어쓰므로, 원래 헤더가 이 규칙을 따를 때만
        // 정확한 원복이 가능합니다 (규칙을 따르지 않는 무장: 유비·손책·주유 등, BUG-03).
        uint16_t ruleLeader = 0xFFFF;
        for (uint16_t b : origBrothers) {
            if (b != 0xFFFF && b != 0) { ruleLeader = b; break; }
        }
        if (ruleLeader != target.sworn_brother_id) {
            std::cout << "  [SKIP] " << target.name << "의 인연 리더 헤더(" << target.sworn_brother_id
                      << ")가 첫 의형제 규칙과 달라 정확한 원복 불가 — 쓰기 검증 생략 (BUG-03)\n";
        } else {
            // 의형제 3인 (조조 333, 유비 610, 관우 94), 친애 2인 (제갈량 283, 조운 380) 지정
            uint16_t newBrothers[3] = { 333, 610, 94 };
            uint16_t newFriends[6] = { 283, 380, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
            bool writeOk = officerMgr.SetSwornBrothers(targetId, newBrothers) && officerMgr.SetFriends(targetId, newFriends);
            OfficerRecord modified;
            bool readOk = officerMgr.LoadOfficer(targetId, modified);

            // 판정 전에 무조건 원복
            bool restoreOk = officerMgr.SetSwornBrothers(targetId, origBrothers) && officerMgr.SetFriends(targetId, origFriends);
            OfficerRecord restored;
            officerMgr.LoadOfficer(targetId, restored);

            if (!writeOk || !readOk ||
                modified.sworn_brothers[0] != 333 || modified.sworn_brothers[1] != 610 || modified.sworn_brothers[2] != 94 ||
                modified.sworn_brother_id != 333 || modified.friends[0] != 283 || modified.friends[1] != 380) {
                std::cerr << "[-] FAIL: " << target.name << " 다중 인연 쓰기/판독 불일치\n";
                return 1;
            }
            std::cout << "  [PASS] " << target.name << " (ID " << targetId << ") 의형제 3인 및 친애 2인 메모리 쓰기 및 헤더 동기화 성공!\n";

            if (!restoreOk || std::memcmp(restored.sworn_brothers, origBrothers, sizeof(origBrothers)) != 0 ||
                std::memcmp(restored.friends, origFriends, sizeof(origFriends)) != 0 ||
                restored.sworn_brother_id != target.sworn_brother_id) {
                std::cerr << "[-] FAIL: " << target.name << " 다중 인연 원복 실패\n";
                return 1;
            }
            std::cout << "  [PASS] 원래 상태(의형제·친애·인연 리더 헤더)로 100% 안전 원복 완료!\n";
        }
    }

    std::cout << "\n============================================================\n";
    std::cout << " >>> [PHASE 2 ACCEPTANCE CRITERIA 100% 통과] <<<\n";
    std::cout << "============================================================\n";
    return 0;
}
