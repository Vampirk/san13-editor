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
    assert(GameVersionDetector::DetectRootBase(driver, rootBase, verDesc));

    OfficerManager officerMgr;
    assert(officerMgr.Initialize(&driver, rootBase));

    // Step 1: 도원결의 3인(유비 610, 관우 94, 장비 418) 상호 의형제 슬롯 검증
    std::cout << "[Step 1] 도원결의 3인 상호 의형제 슬롯 검증\n";
    OfficerRecord liubei, guanyu, zhangfei;
    assert(officerMgr.LoadOfficer(610, liubei));
    assert(officerMgr.LoadOfficer(94, guanyu));
    assert(officerMgr.LoadOfficer(418, zhangfei));

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

    // Step 3: 주인공 강월(700) 다중 인연 쓰기 및 100% 안전 원복 검증
    std::cout << "\n[Step 3] 주인공 강월(700) 다중 인연 쓰기 및 안전 원복 검증\n";
    OfficerRecord kangwol;
    assert(officerMgr.LoadOfficer(700, kangwol));

    uint16_t origBrothers[3];
    uint16_t origFriends[6];
    std::memcpy(origBrothers, kangwol.sworn_brothers, sizeof(origBrothers));
    std::memcpy(origFriends, kangwol.friends, sizeof(origFriends));

    // 강월에게 의형제 3인 (조조 333, 유비 610, 관우 94) 지정
    uint16_t newBrothers[3] = { 333, 610, 94 };
    assert(officerMgr.SetSwornBrothers(700, newBrothers));

    // 강월에게 친애 무장 (제갈량 283, 조운 380) 지정
    uint16_t newFriends[6] = { 283, 380, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    assert(officerMgr.SetFriends(700, newFriends));

    // 재로드 및 쓰기 결과 판독 검증
    OfficerRecord modified;
    assert(officerMgr.LoadOfficer(700, modified));
    assert(modified.sworn_brothers[0] == 333);
    assert(modified.sworn_brothers[1] == 610);
    assert(modified.sworn_brothers[2] == 94);
    assert(modified.sworn_brother_id == 333); // 헤더 리더 동기화 확인
    assert(modified.friends[0] == 283);
    assert(modified.friends[1] == 380);
    std::cout << "  [PASS] 강월 의형제 3인 및 친애 2인 메모리 쓰기 및 헤더 동기화 성공!\n";

    // 원래 상태로 100% 안전 원복
    assert(officerMgr.SetSwornBrothers(700, origBrothers));
    assert(officerMgr.SetFriends(700, origFriends));

    OfficerRecord restored;
    assert(officerMgr.LoadOfficer(700, restored));
    assert(restored.sworn_brothers[0] == origBrothers[0]);
    assert(restored.sworn_brothers[1] == origBrothers[1]);
    assert(restored.sworn_brothers[2] == origBrothers[2]);
    assert(restored.friends[0] == origFriends[0]);
    assert(restored.friends[1] == origFriends[1]);
    std::cout << "  [PASS] 원래 상태로 100% 안전 원복 완료!\n";

    std::cout << "\n============================================================\n";
    std::cout << " >>> [PHASE 2 ACCEPTANCE CRITERIA 100% 통과] <<<\n";
    std::cout << "============================================================\n";
    return 0;
}
