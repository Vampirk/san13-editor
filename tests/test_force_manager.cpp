#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/ForceManager.hpp"
#include "game/SettlementManager.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [Phase 3] 세력 실명화 매핑 (ForceManager) 검증 테스트\n";
    std::cout << "============================================================\n";

    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] Error: San13PK_kr.exe 게임 프로세스를 찾을 수 없습니다.\n";
        return 1;
    }
    std::cout << "[+] 게임 프로세스 연결 성공 (PID: " << driver.GetPid() << ")\n";

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        std::cerr << "[-] Error: RootBase 탐지 실패!\n";
        return 1;
    }
    std::cout << "[+] RootBase 탐지 성공: 0x" << std::hex << rootBase << std::dec << " (" << verDesc << ")\n";

    // 1. ForceManager 초기화 검증
    std::cout << "\n[Step 1] ForceManager 초기화 및 청크 파싱...\n";
    bool initOk = ForceManager::Initialize(&driver, rootBase);
    assert(initOk && "ForceManager 초기화 실패");
    std::cout << " -> ForceManager 초기화 성공!\n";

    // 2. 전체 세력 목록 캐시 검증
    const auto& forces = ForceManager::GetForces();
    assert(forces.size() == ForceMemoryLayout::MAX_FORCE_COUNT && "세력 배열 크기 불일치");

    int validCount = 0;
    std::cout << "\n[Step 2] 190년 시나리오 세력 매핑 목록:\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << " ForceID | 군주ID | 군주 성명       | 세력 표출 명칭\n";
    std::cout << "------------------------------------------------------------\n";
    for (size_t i = 0; i < forces.size(); ++i) {
        if (forces[i].is_valid) {
            validCount++;
            std::cout << "   " << std::setw(3) << forces[i].force_id << "   | "
                      << std::setw(6) << forces[i].ruler_id << " | "
                      << std::setw(15) << std::left << forces[i].ruler_name << " | "
                      << forces[i].force_name << "\n" << std::right;
        }
    }
    std::cout << "------------------------------------------------------------\n";
    std::cout << " -> 총 발견된 활성 세력 수: " << validCount << "개\n";
    assert(validCount >= 10 && "190년 시나리오에서 활성 세력이 최소 10개 이상이어야 합니다.");

    // 3. 주요 군주 실명 매핑 정밀 단언(Assert)
    std::cout << "\n[Step 3] 주요 군주 실명 매핑 Assert 검증...\n";
    
    // 조조 (ID 333) -> 조조군 (Force 0)
    std::string force0 = ForceManager::GetForceName(0);
    std::cout << " - Force 0: " << force0 << " (군주 ID: " << ForceManager::GetForceRulerId(0) << ")\n";
    assert(force0 == "조조군" && "Force 0은 조조군이어야 합니다.");
    assert(ForceManager::GetForceRulerId(0) == 333);

    // 유비 (ID 610) -> 유비군 (Force 1)
    std::string force1 = ForceManager::GetForceName(1);
    std::cout << " - Force 1: " << force1 << " (군주 ID: " << ForceManager::GetForceRulerId(1) << ")\n";
    assert(force1 == "유비군" && "Force 1은 유비군이어야 합니다.");
    assert(ForceManager::GetForceRulerId(1) == 610);

    // 손견 (ID 355) 또는 손책 (ID 359) -> 손가 세력 (Force 2)
    std::string force2 = ForceManager::GetForceName(2);
    std::cout << " - Force 2: " << force2 << " (군주 ID: " << ForceManager::GetForceRulerId(2) << ")\n";
    assert((force2 == "손견군" || force2 == "손책군") && "Force 2는 손견군 또는 손책군이어야 합니다.");

    // 동탁 (ID 480) 또는 이각 (ID 579) -> 서량 세력 (Force 5)
    std::string force5 = ForceManager::GetForceName(5);
    std::cout << " - Force 5: " << force5 << " (군주 ID: " << ForceManager::GetForceRulerId(5) << ")\n";
    assert((force5 == "동탁군" || force5 == "이각군") && "Force 5는 동탁군 또는 이각군이어야 합니다.");

    // 원소 (ID 18) -> 원소군 (Force 7)
    std::string force7 = ForceManager::GetForceName(7);
    std::cout << " - Force 7: " << force7 << " (군주 ID: " << ForceManager::GetForceRulerId(7) << ")\n";
    assert(force7 == "원소군" && "Force 7은 원소군이어야 합니다.");
    assert(ForceManager::GetForceRulerId(7) == 18);

    // 4. 예외/무소속 처리 검증
    std::cout << "\n[Step 4] 무소속 및 예외 ID 검증...\n";
    assert(ForceManager::GetForceName(0xFFFF) == "무소속");
    assert(ForceManager::GetForceName(255) == "무소속");
    assert(!ForceManager::IsValidForce(0xFFFF));
    assert(!ForceManager::IsValidForce(255));
    assert(!ForceManager::IsValidForce(63)); // 미사용 슬롯
    std::cout << " - 0xFFFF 명칭: " << ForceManager::GetForceName(0xFFFF) << " (OK)\n";
    std::cout << " - 255 명칭: " << ForceManager::GetForceName(255) << " (OK)\n";

    // 5. 부락 실데이터 연동 확인 (도시 종속 기반 실명 세력 표출 검증)
    std::cout << "\n[Step 5] 관할 부락과 세력 실명 연동 검증 (미종속 vs 100% 장악 세력)...\n";
    SettlementManager setMgr;
    if (setMgr.Initialize(&driver, rootBase)) {
        std::vector<SettlementRecord> settlements;
        if (setMgr.LoadAllSettlements(settlements)) {
            int sampleCount = 0;
            int neutralCount = 0;
            int affiliatedCount = 0;
            for (const auto& s : settlements) {
                if (s.id < 15) {
                    std::cout << " - 부락 #" << std::setw(2) << s.id << " [" << std::setw(8) << s.name << "] -> 지배: "
                              << s.force_name << " (개발도: " << s.dev_progress << "%)\n";
                    sampleCount++;
                }
                if (s.force_name.find("미종속") != std::string::npos) {
                    neutralCount++;
                } else {
                    affiliatedCount++;
                }
            }
            std::cout << " -> 전체 300개 부락 중 미종속(중립): " << neutralCount << "개, 세력 종속: " << affiliatedCount << "개\n";
            assert(sampleCount > 0 && "부락 샘플 조회 실패");
            assert(neutralCount > 0 && "미종속 중립 부락이 존재해야 합니다.");
        }
    }

    std::cout << "\n============================================================\n";
    std::cout << " [SUCCESS] Phase 3: 세력 실명화 매핑 (ForceManager) 100% 통과!\n";
    std::cout << "============================================================\n";
    return 0;
}
