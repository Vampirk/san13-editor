#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "core/LinuxMemoryDriver.hpp"
#include "game/Officer.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] Error: Game process not found!\n";
        return 1;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        std::cerr << "[-] Error: RootBase detection failed!\n";
        return 1;
    }

    std::cout << "[+] Connected to game process (PID: " << driver.GetPid() 
              << ", RootBase: 0x" << std::hex << rootBase << std::dec << ")\n";

    // 1. 시나리오 일자 확인
    uint16_t year = 0, month = 0, day = 0;
    driver.Read<uint16_t>(rootBase + 0x137A, year);
    driver.Read<uint16_t>(rootBase + 0x1380, month);
    driver.Read<uint16_t>(rootBase + 0x1386, day);
    std::cout << "[+] Current Game Date: " << year << "년 " << month << "월 " << day << "일\n";

    // 2. RootBase + 0x0000 ~ 0x2000 글로벌 제어 영역에서 잠재적인 officer_id (0~800) 탐색
    std::cout << "\n=== [Scan 1] RootBase + 0x1000 ~ 0x2000 Candidate 16-bit IDs ===\n";
    for (uintptr_t offset = 0x1000; offset < 0x2000; offset += 2) {
        uint16_t val = 0;
        driver.Read<uint16_t>(rootBase + offset, val);
        // 만약 유효한 역사무장 ID(예: 0~799)이고 바로 다음이나 이전 패턴이 의미가 있는지 확인
        if (val < 800) {
            // 해당 무장 이름 가져오기
            std::string_view name = GetOfficerKoreanName(val);
            if (!name.empty() && name != "미상") {
                // 특정 오프셋에 무장 ID가 저장된 곳들
                // 특히 0x1370 ~ 0x1400 부근
                if (offset >= 0x1300 && offset <= 0x1500) {
                    std::cout << "  RootBase + 0x" << std::hex << offset << std::dec 
                              << " = " << val << " (" << name << ")\n";
                }
            }
        }
    }

    // 3. 세력(Force) 청크 (RootBase + 0x00ED60, Stride: 0x148, 64개) 검사
    // 플레이어 세력 플래그나 플레이어 무장 ID가 있는지 조사
    std::cout << "\n=== [Scan 2] Force Chunks (RootBase + 0x00ED60) ===\n";
    const uintptr_t FORCE_BASE = rootBase + 0x00ED60;
    const size_t FORCE_STRIDE = 0x148;
    for (int f = 0; f < 64; ++f) {
        uintptr_t forceAddr = FORCE_BASE + f * FORCE_STRIDE;
        uint32_t vtable = 0;
        driver.Read<uint32_t>(forceAddr, vtable);
        if (vtable == 0) continue;

        // 세력 내 여러 필드 덤프
        uint16_t lordId = 0;
        driver.Read<uint16_t>(forceAddr + 0x04, lordId); // 군주 ID 추정

        uint8_t forceBuf[0x40] = {0};
        driver.ReadRaw(forceAddr, forceBuf, sizeof(forceBuf));

        std::cout << "  Force #" << f << " (Addr: 0x" << std::hex << forceAddr << std::dec << "): ";
        for (int i = 0; i < 32; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)forceBuf[i] << " ";
        }
        std::cout << std::dec << "\n";
    }

    // 4. 무장 구조체(2,165명)에서 유일하게 1개만 존재하는 플래그/비트 조사
    // 각 무장의 헤더(0x00 ~ 0x58) 덤프 분석
    std::cout << "\n=== [Scan 3] Scanning Officer Header Flags for Unique / Player Markers ===\n";
    const uintptr_t OFFICER_BASE = rootBase + 0x015EE0;
    const size_t OFFICER_STRIDE = 0x12BC;

    // 무장별 +0x00 ~ +0x14, +0x4E ~ +0x58 중 값이 다른 무장과 차별화되는 것 조사
    // 우선 역사무장 800명 중 activity_flag == 1인 무장들 수집
    int activeCount = 0;
    for (int i = 0; i < 800; ++i) {
        uintptr_t offAddr = OFFICER_BASE + i * OFFICER_STRIDE;
        uint32_t vtable = 0;
        driver.Read<uint32_t>(offAddr, vtable);
        if (vtable != static_cast<uint32_t>(driver.GetModuleBase() + San13::Game::OfficerMemoryLayout::VTABLE_RVA) /* EDGE-56 */) continue;

        uint8_t act = 0;
        driver.Read<uint8_t>(offAddr + 0x4F, act);
        if (act == 1) activeCount++;
    }
    std::cout << "  Active Officers (activity_flag == 1): " << activeCount << "\n";

    return 0;
}
