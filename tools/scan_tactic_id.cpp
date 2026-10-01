/**
 * scan_tactic_id.cpp
 * Step 6.0-2: 무장 오브젝트 내 전법(Tactics) ID 오프셋 탐색
 *
 * 전략:
 *   1) 기준 무장 4인의 +0x50 ~ +0xC0 영역(128B)을 덤프
 *   2) 반에디터에서 확인한 전법 이름 VA 목록과 대조,
 *      실제 전법 ID(작은 정수)로 보이는 uint8_t / uint16_t 후보를 찾음
 *   3) 같은 오프셋에서 4인 모두 다른 값을 가지면서 1~200 범위 이내이면 전법 ID 오프셋 후보
 *
 * 기준 무장 및 인게임 기본 전법:
 *   여포   (ID 635) = 천하무쌍
 *   관우   (ID  94) = 군신
 *   제갈량 (ID 283) = 신산귀모
 *   조조   (ID 333) = 초세지걸
 *   조운   (ID 380) = 용담 (다른 4인과 겹치지 않아야)
 *   장비   (ID 418) = 돌격지휘 (추가 검증)
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"

using namespace San13::Core;
using namespace San13::Game;

static constexpr uintptr_t POOL_OFFSET = 0x015EE0;
static constexpr size_t    STRIDE      = 0x12BC;

// 알려진 전법 이름 (반에디터 바이너리 확인 목록 + 게임 데이터)
// 전법 ID는 보통 1-based 혹은 0-based 작은 정수일 것으로 예상
// 지금은 범위만 판단: 0~255 (1바이트) or 0~65535 (2바이트)
struct OfficerRef {
    const char* name;
    uint16_t id;
    const char* tactic_name;
};

static const OfficerRef REFS[] = {
    { "여포",   635, "천하무쌍" },
    { "관우",    94, "군신"     },
    { "제갈량", 283, "신산귀모" },
    { "조조",   333, "초세지걸" },
    { "조운",   380, "용담"     },
    { "장비",   418, "돌격지휘" },
};

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "프로세스 연결 실패\n";
        return 1;
    }

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);
    std::cout << "RootBase: 0x" << std::hex << root_base << "  (" << verDesc << ")\n\n";

    // 각 무장의 +0x50 ~ +0xC0 (112바이트) 덤프
    const size_t SCAN_START = 0x50;
    const size_t SCAN_END   = 0xC0;
    const size_t SCAN_LEN   = SCAN_END - SCAN_START;

    struct OfficerDump {
        const char* name;
        uint16_t id;
        const char* tactic;
        std::vector<uint8_t> data;
    };

    std::vector<OfficerDump> dumps;

    for (auto& ref : REFS) {
        uintptr_t addr = root_base + POOL_OFFSET + (uintptr_t)ref.id * STRIDE;
        std::vector<uint8_t> buf(SCAN_LEN, 0);
        driver.ReadRaw(addr + SCAN_START, buf.data(), SCAN_LEN);
        dumps.push_back({ ref.name, ref.id, ref.tactic_name, buf });
    }

    // ── 1. Raw Hex 덤프 출력 ──────────────────────────────────────────────
    std::cout << "=== +0x50 ~ +0xBF RAW 덤프 ===\n";
    for (auto& d : dumps) {
        std::cout << "\n[" << d.name << " (ID " << std::dec << d.id
                  << ", 전법=" << d.tactic << ")]\n";
        for (size_t i = 0; i < SCAN_LEN; i++) {
            if (i % 16 == 0)
                std::cout << "  +0x" << std::hex << std::setw(2) << std::setfill('0')
                          << (SCAN_START + i) << ": ";
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << (int)d.data[i] << " ";
            if ((i + 1) % 16 == 0) std::cout << "\n";
        }
        std::cout << "\n";
    }

    // ── 2. 전법 ID 후보 탐색: 1바이트 (uint8_t) 오프셋 기준 ─────────────
    // 모든 무장에서 해당 오프셋 값이 서로 다르고 1~200 범위이면 후보
    std::cout << "\n=== 전법 ID 후보 탐색 (uint8_t, 1~200 범위, 6인 모두 다른 값) ===\n";
    for (size_t off = 0; off < SCAN_LEN; off++) {
        std::set<uint8_t> vals;
        bool allInRange = true;
        for (auto& d : dumps) {
            uint8_t v = d.data[off];
            if (v < 1 || v > 200) { allInRange = false; break; }
            vals.insert(v);
        }
        if (allInRange && vals.size() == dumps.size()) {
            std::cout << "  +0x" << std::hex << std::setw(2) << std::setfill('0')
                      << (SCAN_START + off) << ": [ ";
            for (auto& d : dumps)
                std::cout << d.name << "=" << std::dec << (int)d.data[off] << " ";
            std::cout << "]  ← 유력 후보!\n";
        }
    }

    // ── 3. 전법 ID 후보 탐색: 2바이트 (uint16_t) ─────────────────────────
    std::cout << "\n=== 전법 ID 후보 탐색 (uint16_t LE, 1~500 범위, 6인 모두 다른 값) ===\n";
    for (size_t off = 0; off < SCAN_LEN - 1; off++) {
        std::set<uint16_t> vals;
        bool allInRange = true;
        for (auto& d : dumps) {
            uint16_t v;
            std::memcpy(&v, &d.data[off], 2);
            if (v < 1 || v > 500) { allInRange = false; break; }
            vals.insert(v);
        }
        if (allInRange && vals.size() == dumps.size()) {
            std::cout << "  +0x" << std::hex << std::setw(2) << std::setfill('0')
                      << (SCAN_START + off) << ": [ ";
            for (auto& d : dumps) {
                uint16_t v; std::memcpy(&v, &d.data[off], 2);
                std::cout << d.name << "=" << std::dec << v << " ";
            }
            std::cout << "]  ← 유력 후보 (uint16_t)!\n";
        }
    }

    // ── 4. 상위 8바이트 추가 확인용: +0x50 ~ +0x57 상세 ──────────────────
    std::cout << "\n=== +0x50 ~ +0x5F 첫 16바이트 상세 (10진수) ===\n";
    for (auto& d : dumps) {
        std::cout << "  " << d.name << " (" << d.tactic << "): ";
        for (size_t i = 0; i < 16 && i < SCAN_LEN; i++)
            std::cout << std::dec << std::setw(4) << (int)d.data[i];
        std::cout << "\n";
    }

    return 0;
}
