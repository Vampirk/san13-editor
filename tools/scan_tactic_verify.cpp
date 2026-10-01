/**
 * scan_tactic_verify.cpp
 * Step 6.0-2: 전법 오프셋 검증
 *
 * 전략:
 *   1) 게임 메모리에서 CP949 전법 이름 직접 검색 → VA 특정
 *   2) 제갈량 +0x98=56, 여포 +0x98=? 등의 오프셋 가설 검증
 *   3) 전법 이름 포인터 테이블에서 56번째 항목이 신산귀모인지 확인
 *
 * 보조: +0x53=보유전법수 재확인, +0x50~+0x57 구조 특정
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"

using namespace San13::Core;
using namespace San13::Game;

static constexpr uintptr_t POOL_OFFSET = 0x015EE0;
static constexpr size_t    STRIDE      = 0x12BC;

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
    { "유비",   600, "왕자지풍" },
    { "손권",   448, "만인적"   },
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

    // ── 각 무장 +0x90 ~ +0xB0 덤프 (제갈량 +0x98 가설 검증) ──────────────
    std::cout << "=== 각 무장 +0x90 ~ +0xAF 덤프 (전법 오프셋 가설: +0x98?) ===\n";
    std::cout << std::setw(8) << "오프셋";
    for (auto& r : REFS) std::cout << std::setw(10) << r.name;
    std::cout << "\n";

    std::vector<std::vector<uint8_t>> dumps;
    for (auto& r : REFS) {
        uintptr_t addr = root_base + POOL_OFFSET + (uintptr_t)r.id * STRIDE;
        std::vector<uint8_t> buf(0x40, 0);
        driver.ReadRaw(addr + 0x90, buf.data(), 0x40);
        dumps.push_back(buf);
    }

    for (size_t i = 0; i < 0x20; i++) {
        std::cout << std::hex << "  +0x" << std::setw(2) << std::setfill('0') << (0x90 + i);
        for (auto& d : dumps) std::cout << std::dec << std::setw(10) << (int)d[i];
        std::cout << "\n";
    }

    // ── +0x98 값을 전법 ID로 가정하고 정리 ────────────────────────────────
    std::cout << "\n=== +0x98 값 (전법 ID 가설) ===\n";
    for (size_t i = 0; i < sizeof(REFS)/sizeof(REFS[0]); i++) {
        uint8_t v = dumps[i][8]; // 0x90+8 = 0x98
        std::cout << "  " << REFS[i].name << " (" << REFS[i].tactic_name << "): "
                  << std::dec << (int)v << " (0x" << std::hex << (int)v << ")\n";
    }

    // ── 전법 이름 테이블 스캔: CP949 패턴 직접 검색 ──────────────────────
    std::cout << "\n=== 게임 메모리 CP949 전법 이름 패턴 직접 스캔 ===\n";

    // CP949 인코딩 (미리 계산)
    // 신산귀모: BD C5 BB EA B1 CD B8 F0
    // 천하무쌍: C3 B5 C7 CF B9 AB BD D6
    // 군신: B1 BA BD C5
    struct TacticPattern { const char* name; std::vector<uint8_t> pattern; };
    std::vector<TacticPattern> patterns = {
        { "신산귀모", {0xBD,0xC5,0xBB,0xEA,0xB1,0xCD,0xB8,0xF0} },
        { "천하무쌍", {0xC3,0xB5,0xC7,0xCF,0xB9,0xAB,0xBD,0xD6} },
        { "군신",     {0xB1,0xBA,0xBD,0xC5} },
        { "초세지걸", {0xC3,0xCA,0xBC,0xBC,0xC1,0xF6,0xB0,0xC9} },
        { "만인적",   {0xB8,0xB8,0xC0,0xCE,0xC0,0xFB} },
        { "돌격지휘", {0xB5,0xB8,0xB0,0xDD,0xC1,0xF6,0xC8,0xD4} },
    };

    // /proc/$PID/maps에서 읽기 가능 메모리 영역 확인 후 스캔
    // 간단히 RootBase 주변 큰 청크 스캔
    // 전법 이름이 있는 예상 범위: RootBase 주변 ±50MB
    const size_t CHUNK = 4 * 1024 * 1024; // 4MB씩
    const uintptr_t SCAN_BASE = 0x70000000;
    const uintptr_t SCAN_END  = 0x80000000;

    bool found_any = false;
    std::map<std::string, uintptr_t> tactic_vas;

    for (uintptr_t scan_addr = SCAN_BASE; scan_addr < SCAN_END; scan_addr += CHUNK) {
        std::vector<uint8_t> chunk(CHUNK, 0);
        size_t read_size = CHUNK;
        if (scan_addr + CHUNK > SCAN_END) read_size = SCAN_END - scan_addr;

        if (!driver.ReadRaw(scan_addr, chunk.data(), read_size)) continue;

        for (auto& tp : patterns) {
            if (tactic_vas.count(tp.name)) continue; // 이미 찾음
            auto it = std::search(chunk.begin(), chunk.end(),
                                  tp.pattern.begin(), tp.pattern.end());
            if (it != chunk.end()) {
                uintptr_t va = scan_addr + std::distance(chunk.begin(), it);
                tactic_vas[tp.name] = va;
                std::cout << "  " << tp.name << ": VA=0x" << std::hex << va << "\n";
                found_any = true;
            }
        }

        // 모두 찾으면 종료
        if (tactic_vas.size() == patterns.size()) break;
    }

    if (!found_any) {
        std::cout << "  CP949 패턴 미발견 - 다른 인코딩 또는 압축 저장 가능성\n";
    }

    // ── 전법 이름 VA 기반 포인터 테이블 분석 ─────────────────────────────
    if (tactic_vas.count("신산귀모") && tactic_vas.count("군신")) {
        std::cout << "\n=== 전법 이름 VA 간격 분석 (ID 추산) ===\n";
        for (auto& [name, va] : tactic_vas) {
            std::cout << "  " << name << ": VA=0x" << std::hex << va << "\n";
        }
    }

    // ── 최종: +0x90~+0x9F 전체 오프셋 비교 ───────────────────────────────
    std::cout << "\n=== 8인 전법 ID 후보 오프셋 비교 ===\n";
    std::cout << "모든 무장에서 값이 다르고 1~250 범위인 오프셋:\n";
    for (size_t i = 0; i < 0x20; i++) {
        std::set<uint8_t> vals;
        bool allInRange = true;
        for (auto& d : dumps) {
            uint8_t v = d[i];
            if (v == 0 || v > 250) { allInRange = false; break; }
            vals.insert(v);
        }
        if (allInRange && vals.size() >= 6) { // 8인 중 6인 이상
            std::cout << std::hex << "  +0x" << std::setw(2) << std::setfill('0') << (0x90 + i) << ":";
            for (size_t j = 0; j < sizeof(REFS)/sizeof(REFS[0]); j++) {
                std::cout << " " << REFS[j].name << "=" << std::dec << (int)dumps[j][i];
            }
            std::cout << "\n";
        }
    }

    return 0;
}
