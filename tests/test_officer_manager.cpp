#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [\033[31mFAIL\033[0m] " << msg << " (" << #cond << ")\n"; \
            return false; \
        } else { \
            std::cout << "  [\033[32mPASS\033[0m] " << msg << "\n"; \
        } \
    } while (0)

using namespace San13::Core;
using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 4.3] Officer Manager Domain Unit Test Suite\n";
    std::cout << "============================================================\n\n";

    // 1. 드라이버 초기화 및 프로세스 부착
    std::cout << "[Step 1] 드라이버 초기화 및 삼국지 13 PK 프로세스 부착\n";
    LinuxMemoryDriver driver;
    bool attached = driver.Attach("San13PK_kr.exe") || driver.Attach("san13pk");
    if (!attached) {
        std::cerr << "[-] Error: 게임 프로세스를 찾을 수 없습니다.\n";
        return 1;
    }
    std::cout << "  [PASS] 프로세스 부착 성공 (PID: " << driver.GetPid() << ")\n";

    // 2. 버전 감지 및 RootBase 산출
    std::cout << "\n[Step 2] GameVersionDetector 버전 및 RootBase 산출\n";
    uintptr_t rootBase = 0;
    std::string verDesc;
    bool verOk = GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);
    TEST_ASSERT(verOk, "버전 감지 및 RootBase 산출 성공");
    std::cout << "         -> 버전: " << verDesc << "\n";
    std::cout << "         -> RootBase: 0x" << std::hex << rootBase << std::dec << "\n";

    // 3. OfficerManager 초기화
    std::cout << "\n[Step 3] OfficerManager 초기화\n";
    OfficerManager mgr;
    bool initOk = mgr.Initialize(&driver, rootBase);
    TEST_ASSERT(initOk, "OfficerManager 초기화 성공");

    // 4. 주요 유명 무장 데이터 검증
    std::cout << "\n[Step 4] 주요 무장(관우, 제갈량, 조운, 장비, 여포) 데이터 무결성 검증\n";
    OfficerRecord guanyu, zhugeliang, zhaoyun, zhangfei, lubu;
    TEST_ASSERT(mgr.LoadOfficer(94, guanyu), "관우 (ID 94) 로드 성공");
    TEST_ASSERT(guanyu.stats.base_leadership == 96 && guanyu.stats.base_war == 97, "관우 통솔 96 / 무력 97 검증");

    TEST_ASSERT(mgr.LoadOfficer(283, zhugeliang), "제갈량 (ID 283) 로드 성공");
    TEST_ASSERT(zhugeliang.stats.base_intel == 100 && zhugeliang.stats.base_politics == 95, "제갈량 지력 100 / 정치 95 검증");

    TEST_ASSERT(mgr.LoadOfficer(380, zhaoyun), "조운 (ID 380) 로드 성공");
    TEST_ASSERT(zhaoyun.stats.base_war == 96, "조운 무력 96 검증");

    TEST_ASSERT(mgr.LoadOfficer(418, zhangfei), "장비 (ID 418) 로드 성공");
    TEST_ASSERT(zhangfei.stats.base_war == 98, "장비 무력 98 검증");

    TEST_ASSERT(mgr.LoadOfficer(635, lubu), "여포 (ID 635) 로드 성공");
    TEST_ASSERT(lubu.stats.base_war == 100, "여포 무력 100 검증");

    // 5. Bounds Clamping & 쓰기 보호 통합 검증
    std::cout << "\n[Step 5] 실시간 쓰기 및 Bounds Clamping 검증 (조조 ID 333)\n";
    OfficerRecord caocao;
    mgr.LoadOfficer(333, caocao);
    std::cout << "         -> 조조 현재 무력: " << (int)caocao.stats.base_war << "\n";

    // 5-1. 바닐라 모드: 120 입력 시 100으로 절삭
    mgr.SetWar(333, 120, ClampingPolicy::SafeVanilla);
    mgr.LoadOfficer(333, caocao);
    TEST_ASSERT(caocao.stats.base_war == 100, "Safe 모드: 120 입력 시 100으로 클램핑");

    // 5-2. 한계 돌파 모드: 256 입력 시 255로 포화 연산 고정 (0 오버플로 방지)
    mgr.SetWar(333, 256, ClampingPolicy::LimitBreak);
    mgr.LoadOfficer(333, caocao);
    TEST_ASSERT(caocao.stats.base_war == 255, "LimitBreak 모드: 256 입력 시 255로 포화 연산 고정");

    // 5-3. 음수 입력 시: 1로 보정
    mgr.SetWar(333, -10, ClampingPolicy::SafeVanilla);
    mgr.LoadOfficer(333, caocao);
    TEST_ASSERT(caocao.stats.base_war == 1, "음수(-10) 입력 시 1로 보정");

    // 5-4. 적성 Strict Guard: 5 입력 시 3(S)으로 절삭
    mgr.SetAptitudes(333, 5, 2, 2);
    mgr.LoadOfficer(333, caocao);
    TEST_ASSERT(caocao.stats.spear_apt == 3, "적성 5 입력 시 Strict Guard(3:S)로 절삭");

    // 5-5. 테스트 후 조조 무력 99로 복구
    mgr.SetWar(333, 99, ClampingPolicy::SafeVanilla);
    mgr.LoadOfficer(333, caocao);
    TEST_ASSERT(caocao.stats.base_war == 99, "조조 무력 99 원복 완료");

    // 6. 800명 전원 일괄 로드 및 메모리 무결성 검증
    std::cout << "\n[Step 6] 800명 역사 무장 전원 일괄 파싱 및 메모리 무결성 검증\n";
    std::vector<OfficerRecord> allOfficers;
    bool allLoaded = mgr.LoadAllOfficers(allOfficers, false);
    TEST_ASSERT(allLoaded, "800명 무장 일괄 로드 성공");
    std::cout << "         -> 로드된 역사 무장 수: " << allOfficers.size() << "\n";
    TEST_ASSERT(allOfficers.size() == 800, "정확히 800명 레코드 수집 확인");

    // 800명 ID 연속성 검증
    bool idsContinuous = true;
    for (size_t i = 0; i < allOfficers.size(); i++) {
        if (allOfficers[i].id != i) {
            idsContinuous = false;
            break;
        }
    }
    TEST_ASSERT(idsContinuous, "0부터 799까지 무장 ID 완전 연속성 확인");

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 모든 도메인 로직 및 800명 무장 로드 검증 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> \033[32m[PHASE 4.3 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";

    return 0;
}
