#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <map>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/City.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] Error: 프로세스 연결 실패\n";
        return 1;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, rootBase, verDesc);

    // Range between City end and Force start:
    // RootBase + 0x00C7B8 to RootBase + 0x00ED60 (9,640 B)
    const uintptr_t GAP_START = rootBase + 0x00C7B8;
    const uintptr_t GAP_END   = rootBase + 0x00ED60;
    const size_t GAP_SIZE = GAP_END - GAP_START;

    std::vector<uint8_t> buf(GAP_SIZE);
    if (!driver.ReadRaw(GAP_START, buf.data(), GAP_SIZE)) {
        std::cerr << "[-] Error: Gap 영역 읽기 실패\n";
        return 1;
    }

    std::cout << "========================================================================\n";
    std::cout << " [Gap 1: City ~ Force] 0x" << std::hex << GAP_START << " ~ 0x" << GAP_END 
              << " (" << std::dec << GAP_SIZE << " B) 분석\n";
    std::cout << "========================================================================\n\n";

    // VTable 패턴 검색
    std::map<uint32_t, std::vector<size_t>> vtables;
    for (size_t i = 0; i + 4 <= GAP_SIZE; i += 4) {
        uint32_t val = *reinterpret_cast<const uint32_t*>(&buf[i]);
        // Typically vtables start with 0x72...
        if ((val & 0xFF000000) == 0x72000000) {
            vtables[val].push_back(i);
        }
    }

    std::cout << "[Step 1] 발견된 VTable 포인터 목록:\n";
    for (const auto& [vt, offsets] : vtables) {
        std::cout << "  VTable: 0x" << std::hex << vt << std::dec 
                  << " | 개수: " << offsets.size() << " 개";
        if (offsets.size() >= 2) {
            size_t stride = offsets[1] - offsets[0];
            std::cout << " | 추정 스트라이드: " << stride << " 바이트 (0x" << std::hex << stride << std::dec << ")";
        }
        std::cout << " | 첫 오프셋: +0x" << std::hex << offsets[0] << std::dec << "\n";
    }

    // VTable 0x72919AC4 정밀 분석
    uint32_t targetVt = 0x72919AC4;
    if (vtables.find(targetVt) != vtables.end()) {
        const auto& offsets = vtables[targetVt];
        std::cout << "\n[Step 2] VTable 0x72919AC4 오브젝트 상세 분석 (" << offsets.size() << " 개):\n";
        size_t stride = (offsets.size() >= 2) ? (offsets[1] - offsets[0]) : 32;

        std::cout << "  스트라이드: " << stride << " 바이트\n";
        std::cout << "  헤더/첫 5개 오브젝트 덤프:\n";
        for (size_t idx = 0; idx < std::min<size_t>(5, offsets.size()); idx++) {
            size_t off = offsets[idx];
            std::cout << "  [Obj " << std::setw(3) << idx << " at +0x" << std::hex << off << std::dec << "]: ";
            for (size_t b = 0; b < stride && off + b < GAP_SIZE; b++) {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)buf[off + b] << " ";
            }
            std::cout << std::dec << "\n";
        }

        // 전체 오브젝트 내에서 도시 ID(0 또는 23), 또는 특정 수치가 있는지 확인
        std::cout << "\n[Step 3] 소속/도시 연결 필드 탐색:\n";
        for (size_t idx = 0; idx < offsets.size(); idx++) {
            size_t off = offsets[idx];
            uint16_t id = *reinterpret_cast<const uint16_t*>(&buf[off + 4]);
            // check bytes
            uint8_t city_candidate = buf[off + 6];
            uint8_t force_candidate = buf[off + 7];
            if (idx < 10 || city_candidate == 23 || city_candidate == 0) {
                std::cout << "  Obj " << std::setw(3) << idx 
                          << " | ID: " << id 
                          << " | +06: " << (int)buf[off + 6]
                          << " | +07: " << (int)buf[off + 7]
                          << " | +08: " << *reinterpret_cast<const uint16_t*>(&buf[off + 8])
                          << " | +0A: " << *reinterpret_cast<const uint16_t*>(&buf[off + 10])
                          << " | +0C: " << *reinterpret_cast<const uint16_t*>(&buf[off + 12])
                          << " | +0E: " << *reinterpret_cast<const uint16_t*>(&buf[off + 14])
                          << "\n";
            }
        }
    }

    return 0;
}
