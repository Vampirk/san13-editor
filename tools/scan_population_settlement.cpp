#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/City.hpp"

using namespace San13::Core;
using namespace San13::Game;

struct MemoryRegion {
    uintptr_t start;
    uintptr_t end;
    std::string perms;
    std::string name;
};

std::vector<MemoryRegion> GetProcessRegions(int pid) {
    std::vector<MemoryRegion> regions;
    std::string path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream file(path);
    if (!file.is_open()) return regions;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string addrRange, perms, offset, dev, inode, name;
        if (iss >> addrRange >> perms >> offset >> dev >> inode) {
            std::getline(iss >> std::ws, name);
            size_t dash = addrRange.find('-');
            if (dash != std::string::npos) {
                uintptr_t start = std::stoul(addrRange.substr(0, dash), nullptr, 16);
                uintptr_t end = std::stoul(addrRange.substr(dash + 1), nullptr, 16);
                // Only readable regions, skip read-only code sections (.text)
                if (perms.size() >= 2 && perms[0] == 'r' && (perms[1] == 'w' || name.find("san13") != std::string::npos)) {
                    regions.push_back({start, end, perms, name});
                }
            }
        }
    }
    return regions;
}

int main(int argc, char** argv) {
    std::cout << "========================================================================\n";
    std::cout << " [San13PK Phase 2] Population & Settlement Memory Discovery Scanner\n";
    std::cout << "========================================================================\n\n";

    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] Error: 삼국지 13 PK 프로세스를 찾을 수 없습니다.\n";
        return 1;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        std::cerr << "[-] Error: RootBase 감지 실패\n";
        return 1;
    }

    int pid = driver.GetPid();
    std::cout << "[+] 프로세스 부착 성공 (PID: " << pid << ")\n";
    std::cout << "[+] 게임 버전: " << verDesc << "\n";
    std::cout << "[+] RootBase: 0x" << std::hex << rootBase << std::dec << "\n\n";

    // 1. 타깃 검색 값 정의
    // 양평 (ID 0): 인구 97,120
    // 금성 (ID 23): 인구 222,200, 증가병력 1975, 자금입출 1977, 군량수입 7167, 상업차이 500
    const uint32_t TARGET_YANGPYEONG_POP = 97120;
    const uint32_t TARGET_GEUMSEONG_POP  = 222200;
    const uint32_t TARGET_GEUMSEONG_INC_TROOPS = 1975;
    const uint32_t TARGET_GEUMSEONG_GOLD_INC   = 1977;
    const uint32_t TARGET_GEUMSEONG_PROV_INC   = 7167;
    const uint16_t TARGET_BONUS_COMMERCE       = 500;

    std::cout << "[Step 1] 탐색 타깃 수치 (인게임 실측값):\n";
    std::cout << "  - 양평 인구: " << TARGET_YANGPYEONG_POP << " (0x" << std::hex << TARGET_YANGPYEONG_POP << std::dec << ")\n";
    std::cout << "  - 금성 인구: " << TARGET_GEUMSEONG_POP  << " (0x" << std::hex << TARGET_GEUMSEONG_POP  << std::dec << ")\n";
    std::cout << "  - 금성 증가병력: " << TARGET_GEUMSEONG_INC_TROOPS << " (0x" << std::hex << TARGET_GEUMSEONG_INC_TROOPS << std::dec << ")\n";
    std::cout << "  - 금성 자금입출: " << TARGET_GEUMSEONG_GOLD_INC   << " (0x" << std::hex << TARGET_GEUMSEONG_GOLD_INC   << std::dec << ")\n";
    std::cout << "  - 금성 군량수입: " << TARGET_GEUMSEONG_PROV_INC   << " (0x" << std::hex << TARGET_GEUMSEONG_PROV_INC   << std::dec << ")\n";
    std::cout << "  - 부락 상업 보너스 추정: " << TARGET_BONUS_COMMERCE << " (0x" << std::hex << TARGET_BONUS_COMMERCE << std::dec << ")\n\n";

    // 2. RootBase 근방 (RootBase - 0x10000 ~ RootBase + 0x100000) 우선 정밀 스캔
    std::cout << "[Step 2] RootBase 주변 영역 (-64KB ~ +1MB) 고정밀 스캔...\n";
    uintptr_t scanStart = (rootBase > 0x10000) ? (rootBase - 0x10000) : 0;
    uintptr_t scanEnd   = rootBase + 0x100000;
    size_t scanSize     = scanEnd - scanStart;

    std::vector<uint8_t> buffer(scanSize);
    if (driver.ReadRaw(scanStart, buffer.data(), scanSize)) {
        std::vector<uintptr_t> yangpopAddrs;
        std::vector<uintptr_t> geumpopAddrs;
        std::vector<uintptr_t> incTroopsAddrs;
        std::vector<uintptr_t> goldIncAddrs;
        std::vector<uintptr_t> provIncAddrs;

        for (size_t i = 0; i + 4 <= scanSize; i += 2) {
            uint32_t val32 = *reinterpret_cast<const uint32_t*>(&buffer[i]);
            uint16_t val16 = *reinterpret_cast<const uint16_t*>(&buffer[i]);
            uintptr_t curAddr = scanStart + i;

            if (val32 == TARGET_YANGPYEONG_POP) yangpopAddrs.push_back(curAddr);
            if (val32 == TARGET_GEUMSEONG_POP)  geumpopAddrs.push_back(curAddr);
            if (val32 == TARGET_GEUMSEONG_INC_TROOPS || val16 == TARGET_GEUMSEONG_INC_TROOPS) incTroopsAddrs.push_back(curAddr);
            if (val32 == TARGET_GEUMSEONG_GOLD_INC   || val16 == TARGET_GEUMSEONG_GOLD_INC)   goldIncAddrs.push_back(curAddr);
            if (val32 == TARGET_GEUMSEONG_PROV_INC   || val16 == TARGET_GEUMSEONG_PROV_INC)   provIncAddrs.push_back(curAddr);
        }

        std::cout << "  -> 양평 인구(97,120) 발견 수: " << yangpopAddrs.size() << " 곳\n";
        for (auto a : yangpopAddrs) {
            int64_t diff = (int64_t)a - (int64_t)rootBase;
            std::cout << "     주소: 0x" << std::hex << a << " (RootBase " << (diff >= 0 ? "+" : "") << diff << " = 0x" << diff << std::dec << ")\n";
        }

        std::cout << "  -> 금성 인구(222,200) 발견 수: " << geumpopAddrs.size() << " 곳\n";
        for (auto a : geumpopAddrs) {
            int64_t diff = (int64_t)a - (int64_t)rootBase;
            std::cout << "     주소: 0x" << std::hex << a << " (RootBase " << (diff >= 0 ? "+" : "") << diff << " = 0x" << diff << std::dec << ")\n";
        }

        // 양평 인구와 금성 인구 간의 오프셋 상관관계 분석
        for (auto y : yangpopAddrs) {
            for (auto g : geumpopAddrs) {
                int64_t dist = (int64_t)g - (int64_t)y;
                if (dist > 0 && dist % 23 == 0) {
                    std::cout << "  [★ 유력 배열 발견!] 양평 주소 0x" << std::hex << y 
                              << " <-> 금성 주소 0x" << g 
                              << " | 거리: " << std::dec << dist 
                              << " B (23개 간격 스트라이드 = " << (dist / 23) << " B)\n";
                }
            }
        }

        std::cout << "  -> 금성 증가병력(1975) 발견 수: " << incTroopsAddrs.size() << " 곳\n";
        for (size_t k = 0; k < std::min<size_t>(5, incTroopsAddrs.size()); k++) {
            uintptr_t a = incTroopsAddrs[k];
            int64_t diff = (int64_t)a - (int64_t)rootBase;
            std::cout << "     주소: 0x" << std::hex << a << " (RootBase " << (diff >= 0 ? "+" : "") << diff << " = 0x" << diff << std::dec << ")\n";
        }

        std::cout << "  -> 금성 자금수입(1977) 발견 수: " << goldIncAddrs.size() << " 곳\n";
        for (size_t k = 0; k < std::min<size_t>(5, goldIncAddrs.size()); k++) {
            uintptr_t a = goldIncAddrs[k];
            int64_t diff = (int64_t)a - (int64_t)rootBase;
            std::cout << "     주소: 0x" << std::hex << a << " (RootBase " << (diff >= 0 ? "+" : "") << diff << " = 0x" << diff << std::dec << ")\n";
        }

        std::cout << "  -> 금성 군량수입(7167) 발견 수: " << provIncAddrs.size() << " 곳\n";
        for (size_t k = 0; k < std::min<size_t>(5, provIncAddrs.size()); k++) {
            uintptr_t a = provIncAddrs[k];
            int64_t diff = (int64_t)a - (int64_t)rootBase;
            std::cout << "     주소: 0x" << std::hex << a << " (RootBase " << (diff >= 0 ? "+" : "") << diff << " = 0x" << diff << std::dec << ")\n";
        }
    } else {
        std::cerr << "[-] Error: RootBase 근방 메모리 읽기 실패\n";
    }

    // 3. 힙 및 전역 데이터 영역 전체 대상 광역 스캔 (/proc/<pid>/maps)
    std::cout << "\n[Step 3] 가상 메모리 전역 데이터 세그먼트 전수 스캔...\n";
    auto regions = GetProcessRegions(pid);
    std::cout << "  총 판독 가능 세그먼트: " << regions.size() << " 개\n";

    size_t totalYang = 0;
    size_t totalGeum = 0;
    for (const auto& reg : regions) {
        size_t rsize = reg.end - reg.start;
        // 너무 거대한 힙(128MB 초과)은 안전을 위해 청크 단위로 나누거나 스킵
        if (rsize > 64 * 1024 * 1024) continue;

        std::vector<uint8_t> rbuf(rsize);
        if (!driver.ReadRaw(reg.start, rbuf.data(), rsize)) continue;

        for (size_t i = 0; i + 4 <= rsize; i += 4) {
            uint32_t val = *reinterpret_cast<const uint32_t*>(&rbuf[i]);
            if (val == TARGET_YANGPYEONG_POP) {
                totalYang++;
                std::cout << "  [양평 인구 발견] 0x" << std::hex << (reg.start + i) 
                          << " (세그먼트: " << reg.name << " " << reg.perms << ")\n";
            }
            if (val == TARGET_GEUMSEONG_POP) {
                totalGeum++;
                std::cout << "  [금성 인구 발견] 0x" << std::hex << (reg.start + i) 
                          << " (세그먼트: " << reg.name << " " << reg.perms << ")\n";
            }
        }
    }
    std::cout << "  전역 스캔 완료 (양평 총 " << totalYang << " 건, 금성 총 " << totalGeum << " 건 발견)\n\n";

    // 4. 도시와 세력 사이의 미확인 갭 영역 분석
    // City 끝: RootBase + 0x00C7B8
    // Force 시작: RootBase + 0x00ED60 (차이: 9,640 바이트)
    std::cout << "[Step 4] 도시~세력 간 미확인 영역(Gap 1: +0x00C7B8 ~ +0x00ED60, 9,640 B) 구조 분석\n";
    uintptr_t gap1Start = rootBase + 0x00C7B8;
    size_t gap1Size = 0x00ED60 - 0x00C7B8;
    std::vector<uint8_t> gap1Buf(gap1Size);
    if (driver.ReadRaw(gap1Start, gap1Buf.data(), gap1Size)) {
        // 비 0 바이트 비율 및 구조 분석
        size_t nonZero = 0;
        for (uint8_t b : gap1Buf) if (b != 0) nonZero++;
        std::cout << "  Gap 1 크기: " << gap1Size << " 바이트 (유효 데이터 바이트: " << nonZero << " / " << gap1Size << ")\n";

        // 이 영역 내에 60개 단위 또는 100+개 단위 스트라이드가 있는지 검사
        std::cout << "  Gap 1 선두 64바이트 덤프:\n    ";
        for (size_t k = 0; k < 64 && k < gap1Size; k++) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)gap1Buf[k] << " ";
            if ((k + 1) % 16 == 0 && k + 1 < 64) std::cout << "\n    ";
        }
        std::cout << std::dec << "\n";
    }

    // 5. 도시 내부 오프셋 재검토 (인구가 스케일링되어 있을 가능성: 9712 or 971 / 22220 or 2222)
    std::cout << "\n[Step 5] 도시 368바이트 내부 단위 변환(Scale) 가설 검증\n";
    San13CityRaw raw0, raw23;
    uintptr_t city0Addr = rootBase + CityMemoryLayout::CHUNK_OFFSET;
    uintptr_t city23Addr = city0Addr + 23 * CityMemoryLayout::OBJECT_STRIDE;
    driver.ReadRaw(city0Addr, &raw0, sizeof(raw0));
    driver.ReadRaw(city23Addr, &raw23, sizeof(raw23));

    const uint8_t* b0 = reinterpret_cast<const uint8_t*>(&raw0);
    const uint8_t* b23 = reinterpret_cast<const uint8_t*>(&raw23);

    std::cout << "  양평(0) vs 금성(23) 도시 청크 내 비율(222200 / 97120 = 2.2878)과 일치하는 수치 쌍 검색:\n";
    for (size_t off = 0; off + 4 <= sizeof(San13CityRaw); off += 2) {
        uint32_t u0 = *reinterpret_cast<const uint32_t*>(b0 + off);
        uint32_t u23 = *reinterpret_cast<const uint32_t*>(b23 + off);
        if (u0 > 100 && u23 > 100) {
            double ratio = (double)u23 / (double)u0;
            if (ratio >= 2.25 && ratio <= 2.32) {
                std::cout << "    [후보 발견] 오프셋 +0x" << std::hex << off << std::dec 
                          << " | 양평: " << u0 << ", 금성: " << u23 
                          << " (비율: " << ratio << ")\n";
            }
        }
    }

    std::cout << "\n========================================================================\n";
    std::cout << " 스캔 완료. 분석 결과를 바탕으로 보고서를 작성합니다.\n";
    std::cout << "========================================================================\n";

    return 0;
}
