#include "core/LinuxMemoryDriver.hpp"
#include <iostream>
#include <cassert>
#include <vector>

// 간단한 단위 테스트 매크로
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

// Test 1: 미실행 프로세스 부착 시도 (EDGE-09)
bool Test_NonExistentProcess(IMemoryDriver& driver) {
    std::cout << "[Test 1] EDGE-09: 존재하지 않는 프로세스 부착 시도 테스트\n";
    bool attached = driver.Attach("Definitely_Not_A_Real_Game_Process_99999");
    TEST_ASSERT(!attached, "미실행 프로세스에 대해 Attach는 false를 반환해야 함");
    TEST_ASSERT(driver.GetPid() <= 0, "PID는 0 이하여야 함");
    TEST_ASSERT(!driver.IsAlive(), "IsAlive()는 false를 반환해야 함");
    return true;
}

// Test 2: 유효하지 않은 주소 및 널 포인터 방어 테스트
bool Test_InvalidAddressGuards(IMemoryDriver& driver) {
    std::cout << "\n[Test 2] 메모리 안전성 가드 (0번지, 널버퍼, 크기 0) 테스트\n";
    uint8_t dummy = 0;
    TEST_ASSERT(!driver.ReadRaw(0, &dummy, 1), "0번지 주소 읽기 시도시 false 반환");
    TEST_ASSERT(!driver.ReadRaw(0x7fffffffffff, nullptr, 10), "nullptr 버퍼 읽기 시도시 false 반환");
    TEST_ASSERT(!driver.ReadRaw(0x7fffffffffff, &dummy, 0), "크기 0 읽기 시도시 false 반환");
    TEST_ASSERT(!driver.WriteRaw(0, &dummy, 1), "0번지 주소 쓰기 시도시 false 반환");
    TEST_ASSERT(!driver.WriteRaw(0x7fffffffffff, nullptr, 10), "nullptr 버퍼 쓰기 시도시 false 반환");
    return true;
}

// Test 3: 실제 실행 중인 San13PK_kr.exe 프로세스 부착 및 검증
bool Test_LiveProcessAttach(IMemoryDriver& driver) {
    std::cout << "\n[Test 3] 실제 게임 프로세스 부착 (San13PK_kr.exe) 테스트\n";
    bool attached = driver.Attach("San13PK_kr.exe");
    if (!attached) {
        // 소문자로 재시도
        attached = driver.Attach("san13pk");
    }
    TEST_ASSERT(attached, "실행 중인 삼국지 13 PK 프로세스에 Attach 성공");
    TEST_ASSERT(driver.GetPid() > 0, "유효한 PID 획득 확인");
    std::cout << "         -> 감지된 PID: " << driver.GetPid() << "\n";
    TEST_ASSERT(driver.IsAlive(), "프로세스 생존 상태 확인 (IsAlive == true)");

    uintptr_t base = driver.GetModuleBase();
    TEST_ASSERT(base != 0, "유효한 모듈 베이스 주소 획득");
    std::cout << "         -> WoW64 Base Address: 0x" << std::hex << base << std::dec << "\n";

    return true;
}

// Test 4: PE 헤더 파싱 및 시그니처 검증 (MZ, PE00)
bool Test_PeHeaderVerification(IMemoryDriver& driver) {
    std::cout << "\n[Test 4] PE 헤더 시그니처 무결성 검증\n";
    uintptr_t base = driver.GetModuleBase();
    
    // 1. DOS Header Magic ('MZ' = 0x5A4D)
    uint16_t dos_magic = 0;
    bool read_magic = driver.Read<uint16_t>(base, dos_magic);
    TEST_ASSERT(read_magic, "DOS 헤더 Magic 읽기 성공");
    TEST_ASSERT(dos_magic == 0x5A4D, "MZ 매직 시그니처(0x5A4D) 일치 확인");

    // 2. e_lfanew offset (at base + 0x3C)
    uint32_t e_lfanew = 0;
    bool read_lfanew = driver.Read<uint32_t>(base + 0x3C, e_lfanew);
    TEST_ASSERT(read_lfanew, "e_lfanew 오프셋 읽기 성공");
    TEST_ASSERT(e_lfanew > 0 && e_lfanew < 0x1000, "e_lfanew 범위 유효성 확인");

    // 3. NT Signature ('PE\0\0' = 0x00004550)
    uint32_t pe_sig = 0;
    bool read_pe = driver.Read<uint32_t>(base + e_lfanew, pe_sig);
    TEST_ASSERT(read_pe, "NT 헤더 시그니처 읽기 성공");
    TEST_ASSERT(pe_sig == 0x00004550, "PE\\0\\0 시그니처 일치 확인");

    // 4. TimeDateStamp (at base + e_lfanew + 8)
    uint32_t timestamp = 0;
    bool read_ts = driver.Read<uint32_t>(base + e_lfanew + 8, timestamp);
    TEST_ASSERT(read_ts, "PE TimeDateStamp 읽기 성공");
    std::cout << "         -> 읽은 TimeDateStamp: 0x" << std::hex << timestamp << std::dec << "\n";
    TEST_ASSERT(timestamp == 0x5E3BA719, "한국어 PK 최신판(1.0.2) 타임스탬프와 일치 확인");

    return true;
}

// Test 5: 분리(Detach) 및 재연결(Re-attach) 라이프사이클 테스트 (EDGE-11)
bool Test_LifecycleDetachReattach(IMemoryDriver& driver) {
    std::cout << "\n[Test 5] EDGE-11: Detach 및 Re-attach 라이프사이클 테스트\n";
    int oldPid = driver.GetPid();
    driver.Detach();

    TEST_ASSERT(driver.GetPid() <= 0, "Detach 후 PID는 0 이하여야 함");
    TEST_ASSERT(!driver.IsAlive(), "Detach 후 IsAlive()는 false여야 함");
    TEST_ASSERT(driver.GetModuleBase() == 0, "Detach 후 ModuleBase는 0이어야 함");

    // 재부착
    bool reattached = driver.Attach("San13PK_kr.exe");
    if (!reattached) reattached = driver.Attach("san13pk");
    TEST_ASSERT(reattached, "재부착(Re-attach) 성공");
    TEST_ASSERT(driver.GetPid() == oldPid, "동일 프로세스 PID 재연결 확인");
    TEST_ASSERT(driver.IsAlive(), "재연결 후 IsAlive() 정상 확인");

    return true;
}

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 4.1] Core Memory Driver Unit Test Suite\n";
    std::cout << "============================================================\n\n";

    LinuxMemoryDriver driver;

    int passed = 0;
    int total = 5;

    if (Test_NonExistentProcess(driver)) passed++;
    if (Test_InvalidAddressGuards(driver)) passed++;
    if (Test_LiveProcessAttach(driver)) passed++;
    if (Test_PeHeaderVerification(driver)) passed++;
    if (Test_LifecycleDetachReattach(driver)) passed++;

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: " << passed << " / " << total << " 개 테스트 세트 통과\n";
    std::cout << "============================================================\n";

    if (passed == total) {
        std::cout << ">>> \033[32m[PHASE 4.1 ACCEPTANCE CRITERIA 100% 통과]\033[0m <<<\n\n";
        return 0;
    } else {
        std::cerr << ">>> \033[31m[PHASE 4.1 실패]\033[0m <<<\n\n";
        return 1;
    }
}
