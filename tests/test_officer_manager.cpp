#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include <iostream>
#include <cstring>
#include <iomanip>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [\033[31mFAIL\033[0m] " << msg << " (" << #cond << ")\n"; \
            return 1; /* main()에서 false는 exit 0(성공)이 되므로 1 반환 (TECH-05) */ \
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
    // 원본 능력치/적성 블록(+0x15 ~ +0x24, 16바이트)을 그대로 보관했다가 판정 전에 바이트 단위로 원복합니다.
    // (과거 구현은 무력을 99로 고정 '복구'하고 적성은 원복하지 않아 실제 게임 데이터를 변경했음)
    const uintptr_t statsAddr = mgr.GetOfficerAddress(333) + OfficerMemoryLayout::STATS_OFFSET;
    San13OfficerLiveStats originalStats{};
    TEST_ASSERT(driver.Read<San13OfficerLiveStats>(statsAddr, originalStats), "조조 원본 능력치 블록 스냅샷");
    std::cout << "         -> 조조 현재 무력: " << (int)originalStats.base_war << "\n";

    OfficerRecord safeClamp, limitClamp, negClamp, aptClamp;
    mgr.SetWar(333, 120, ClampingPolicy::SafeVanilla);   mgr.LoadOfficer(333, safeClamp);
    mgr.SetWar(333, 256, ClampingPolicy::LimitBreak);    mgr.LoadOfficer(333, limitClamp);
    mgr.SetWar(333, -10, ClampingPolicy::SafeVanilla);   mgr.LoadOfficer(333, negClamp);
    mgr.SetAptitudes(333, 5, 2, 2);                      mgr.LoadOfficer(333, aptClamp);

    // 판정 전에 무조건 원복
    San13OfficerLiveStats restoredStats{};
    bool restoreOk = driver.Write<San13OfficerLiveStats>(statsAddr, originalStats) &&
                     driver.Read<San13OfficerLiveStats>(statsAddr, restoredStats) &&
                     std::memcmp(&restoredStats, &originalStats, sizeof(originalStats)) == 0;

    TEST_ASSERT(safeClamp.stats.base_war == 100, "Safe 모드: 120 입력 시 100으로 클램핑");
    TEST_ASSERT(limitClamp.stats.base_war == 255, "LimitBreak 모드: 256 입력 시 255로 포화 연산 고정");
    TEST_ASSERT(negClamp.stats.base_war == 1, "음수(-10) 입력 시 1로 보정");
    TEST_ASSERT(aptClamp.stats.spear_apt == 3, "적성 5 입력 시 Strict Guard(3:S)로 절삭");
    TEST_ASSERT(restoreOk, "조조 능력치·적성 원본 16바이트 원복 완료");

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
