#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerNameTable.hpp"
#include "game/City.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) return 1;

    // 190년 시나리오의 도시 군주 확인:
    // 도시 0: 양평 -> 공손도 (ID 176)
    // 도시 1: 북평 -> 공손찬 (ID 174)
    // 도시 2: 계   -> 유우 (ID 595)
    // 도시 3: 남피 -> 원소 (ID 18)
    // 도시 7: 업   -> 한복 (ID 115)
    // 도시 10: 북해 -> 공융 (ID 184)
    // 도시 15: 진류 -> 조조 (ID 333)
    // 도시 18: 낙양 -> 동탁 (ID 480)
    // 도시 22: 완   -> 원술 (ID 16)
    // 도시 24: 양양 -> 유표 (ID 611)

    struct CityExpectation {
        int cityId;
        const char* cityName;
        uint16_t expectedLord;
        const char* expectedLordName;
    } expected[] = {
        { 0,  "양평", 176, "공손도" },
        { 1,  "북평", 174, "공손찬" },
        { 2,  "계",   595, "유우" },
        { 3,  "남피",  18, "원소" },
        { 7,  "업",   115, "한복" },
        { 10, "북해", 184, "공융" },
        { 15, "진류", 333, "조조" },
        { 18, "낙양", 480, "동탁" },
        { 22, "완",    16, "원술" },
        { 24, "양양", 611, "유표" },
    };

    std::cout << "=== 도시 구조체 (0x170 바이트) 내 군주 ID 검색 ===\n";
    for (const auto& exp : expected) {
        uintptr_t cityAddr = rootBase + CityMemoryLayout::CHUNK_OFFSET + (exp.cityId * CityMemoryLayout::OBJECT_STRIDE);
        std::vector<uint8_t> buf(CityMemoryLayout::OBJECT_STRIDE);
        driver.ReadRaw(cityAddr, buf.data(), buf.size());

        std::cout << "도시 " << exp.cityId << " [" << exp.cityName << "] (예상 군주: " << exp.expectedLordName << " " << exp.expectedLord << "): ";
        std::vector<size_t> matchedOffsets;
        for (size_t off = 0; off + 2 <= buf.size(); off += 2) {
            uint16_t val = *reinterpret_cast<uint16_t*>(&buf[off]);
            if (val == exp.expectedLord) {
                matchedOffsets.push_back(off);
            }
        }
        for (size_t o : matchedOffsets) {
            std::cout << "+0x" << std::hex << o << std::dec << " ";
        }
        if (matchedOffsets.empty()) std::cout << "(일치 오프셋 없음)";
        std::cout << "\n";
    }

    return 0;
}
