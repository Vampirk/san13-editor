/**
 * scan_tactic_bitmap.cpp
 * Step 6.0-2: 전법 비트셋 가설 검증
 *
 * 가설: 무장이 보유한 전법 목록은 전법 ID를 비트 위치로 인코딩한 비트맵 방식으로 저장
 *   - 전법 ID N을 가지면: data[N/8] 의 (N%8) 번째 비트 == 1
 *
 * 전략:
 *   1) 여포(천하무쌍), 관우(군신), 제갈량(신산귀모), 조조(초세지걸)의 +0x50 ~ +0x12BB 전체 덤프
 *   2) 4인의 XOR 차이 비트 위치 분석 → 각 전법에 해당하는 비트 후보 특정
 *   3) 비트 위치 N이 1인 무장 = 전법 N 보유자와 일치하는지 교차검증
 *
 * 추가: +0x53 = 보유 전법 수(1~2) 가설 검증 (다른 무장 추가 샘플링)
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <set>
#include <map>
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
    int tactic_id_guess; // 반에디터 바이너리에서 확인한 전법 문자열 VA 기반 추정 순서
};

// 반에디터에서 확인된 전법 이름 순서 (VA 0x51400 일대, 실측):
// 군신=~1, 천하무쌍=~15?, 신산귀모=~?, 초세지걸=~?
// 정확한 ID는 미확정 → 이번 실험으로 확정 목표
static const OfficerRef REFS[] = {
    { "여포",   635, "천하무쌍", -1 },
    { "관우",    94, "군신",     -1 },
    { "제갈량", 283, "신산귀모", -1 },
    { "조조",   333, "초세지걸", -1 },
    { "조운",   380, "용담",     -1 },
    { "장비",   418, "돌격지휘", -1 },
    // 전법이 명확히 다른 추가 무장들
    { "유비",   600, "왕자지풍", -1 },
    { "손권",   448, "만인적",   -1 },
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

    // +0x50 ~ +0x150 (256바이트) 덤프 — 좀 더 넓게
    const size_t SCAN_START = 0x50;
    const size_t SCAN_LEN   = 0x120; // 288바이트

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

    // ── +0x50 ~ +0x5F 첫 16바이트 테이블 ──────────────────────────────────
    std::cout << "=== +0x50 ~ +0x5F 상세 (10진수) ===\n";
    std::cout << std::setw(8) << "오프셋";
    for (auto& d : dumps) std::cout << std::setw(10) << d.name;
    std::cout << "\n";
    for (size_t i = 0; i < 16; i++) {
        std::cout << std::hex << "  +0x" << std::setw(2) << std::setfill('0') << (SCAN_START + i);
        for (auto& d : dumps) std::cout << std::dec << std::setw(10) << (int)d.data[i];
        std::cout << "\n";
    }

    // ── 비트셋 가설 검증: 각 무장의 세트된 비트 목록 ──────────────────────
    std::cout << "\n=== 비트셋 분석: 각 무장 세트 비트(전법ID후보, 1-indexed) ===\n";
    std::vector<std::set<int>> bit_sets;
    for (auto& d : dumps) {
        std::set<int> bits;
        for (size_t b = 0; b < d.data.size(); b++) {
            uint8_t byte = d.data[b];
            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << bit)) {
                    bits.insert((int)(b * 8 + bit + 1));
                }
            }
        }
        bit_sets.push_back(bits);
        std::cout << "  " << d.name << " (" << d.tactic << "): ";
        int cnt = 0;
        for (int x : bits) {
            if (cnt++ >= 40) { std::cout << "..."; break; }
            std::cout << x << " ";
        }
        std::cout << "\n";
    }

    // ── 무장별 고유 비트 (다른 무장은 0, 해당 무장만 1) ───────────────────
    std::cout << "\n=== 각 무장 고유 비트(다른 7명 모두 0) - 전법 ID 유력 후보 ===\n";
    for (size_t i = 0; i < dumps.size(); i++) {
        std::set<int> unique_bits;
        for (int bit : bit_sets[i]) {
            bool unique = true;
            for (size_t j = 0; j < dumps.size(); j++) {
                if (j == i) continue;
                if (bit_sets[j].count(bit)) { unique = false; break; }
            }
            if (unique) unique_bits.insert(bit);
        }
        std::cout << "  " << dumps[i].name << " (" << dumps[i].tactic << ") 고유: ";
        for (int b : unique_bits) std::cout << b << " ";
        std::cout << "\n";
    }

    // ── +0x53 검증: 보유 전법 수 가설 ────────────────────────────────────
    std::cout << "\n=== +0x53 (오프셋 3) = 보유 전법 수? ===\n";
    for (auto& d : dumps) {
        std::cout << "  " << d.name << ": " << (int)d.data[3] << "\n";
    }

    // ── +0x56 검증: 성격 매핑? ────────────────────────────────────────────
    std::cout << "\n=== +0x56 (오프셋 6) vs +0x4E 성격 비교 ===\n";
    for (auto& d : dumps) {
        // +0x4E는 SCAN_START(0x50) 기준이 아니라 오프셋 자체이므로 별도 읽기
        uintptr_t obj_base = root_base + POOL_OFFSET + (uintptr_t)d.id * STRIDE;
        uint8_t personality = 0;
        driver.ReadRaw(obj_base + 0x4E, &personality, 1);
        static const char* PERS[] = {"소심","냉정","대담","저돌"};
        std::cout << "  " << d.name << ": +0x56=" << (int)d.data[6]
                  << " / 성격(+0x4E)=" << (int)personality
                  << " (" << (personality < 4 ? PERS[personality] : "?") << ")\n";
    }

    // ── 추가: 전법이 0x58 이후 슬롯 배열인지 확인 ────────────────────────
    // 삼국지13 무장은 최대 3개 전법 보유
    // 슬롯 배열이라면 +0x58, +0x5A, +0x5C 에 uint16_t x3 또는
    // +0x58, +0x59, +0x5A 에 uint8_t x3 이어야 함
    std::cout << "\n=== +0x58 ~ +0x63 전법 슬롯 배열 가설 ===\n";
    std::cout << "(uint8_t 기준)\n";
    for (size_t i = 8; i < 20; i++) {
        std::cout << std::hex << "  +0x" << std::setw(2) << std::setfill('0') << (SCAN_START + i);
        for (auto& d : dumps) std::cout << std::dec << std::setw(10) << (int)d.data[i];
        std::cout << "\n";
    }

    std::cout << "\n(uint16_t LE 기준)\n";
    for (size_t i = 8; i < 20; i += 2) {
        std::cout << std::hex << "  +0x" << std::setw(2) << std::setfill('0') << (SCAN_START + i);
        for (auto& d : dumps) {
            uint16_t v; std::memcpy(&v, &d.data[i], 2);
            std::cout << std::dec << std::setw(10) << (int)v;
        }
        std::cout << "\n";
    }

    return 0;
}
