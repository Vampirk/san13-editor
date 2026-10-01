#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/City.hpp"
#include "game/CityManager.hpp"

using namespace San13::Core;
using namespace San13::Game;

static const char* GetKnownLabel(uint16_t offset) {
    switch (offset) {
        case 0x00: return "vtable 포인터";
        case 0x04: return "소속 세력 ID (force_id)";
        case 0x06: return "태수 무장 ID (governor)";
        case 0x08: return "군주 무장 ID (lord)";
        case 0x58: return "[★ 확정] 내구도 (durability)";
        case 0x5C: return "[★ 확정] 자금 (gold)";
        case 0x60: return "[★ 확정] 군량 (provisions)";
        case 0x64: return "[★ 확정] 주둔 병력 (troops)";
        default: return "";
    }
}

// 단일 도시 368바이트(0x170) 상세 덤프 출력
void DumpSingleCity(uint8_t cityId, const uint8_t* buf, size_t size) {
    std::cout << "========================================================================================\n";
    std::cout << " [도시 상세 덤프] ID " << (int)cityId << " : " << GetCityName(cityId) << "\n";
    std::cout << " 오브젝트 크기: " << size << " 바이트 (0x" << std::hex << size << std::dec << ")\n";
    std::cout << "========================================================================================\n";
    std::cout << std::left
              << std::setw(8)  << "오프셋"
              << std::setw(14) << "Hex (4B)"
              << std::setw(18) << "uint8 x 4"
              << std::setw(16) << "uint16 x 2"
              << std::setw(12) << "uint32"
              << "기준점 / 알려진 필드\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (size_t off = 0; off < size; off += 4) {
        uint8_t b0 = buf[off];
        uint8_t b1 = (off + 1 < size) ? buf[off + 1] : 0;
        uint8_t b2 = (off + 2 < size) ? buf[off + 2] : 0;
        uint8_t b3 = (off + 3 < size) ? buf[off + 3] : 0;

        uint16_t u16_0 = b0 | (b1 << 8);
        uint16_t u16_1 = b2 | (b3 << 8);
        uint32_t u32   = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

        // Hex 문자열 생성
        std::ostringstream ossHex;
        ossHex << std::hex << std::setfill('0') << std::uppercase
               << std::setw(2) << (int)b0 << " "
               << std::setw(2) << (int)b1 << " "
               << std::setw(2) << (int)b2 << " "
               << std::setw(2) << (int)b3;

        // uint8 4개 문자열
        std::ostringstream ossU8;
        ossU8 << std::dec << (int)b0 << "," << (int)b1 << "," << (int)b2 << "," << (int)b3;

        // uint16 2개 문자열
        std::ostringstream ossU16;
        ossU16 << std::dec << u16_0 << ", " << u16_1;

        char offStr[16];
        std::snprintf(offStr, sizeof(offStr), "+0x%03X", (unsigned int)off);

        const char* label = GetKnownLabel(static_cast<uint16_t>(off));

        std::cout << std::left
                  << std::setw(10) << offStr
                  << std::setw(14) << ossHex.str()
                  << std::setw(18) << ossU8.str()
                  << std::setw(16) << ossU16.str()
                  << std::setw(12) << u32
                  << label << "\n";
    }
    std::cout << "========================================================================================\n\n";
}

// 다중 도시 동시 비교 (Diff / Compare 모드)
void CompareCities(const std::vector<uint8_t>& cityIds, const std::vector<std::vector<uint8_t>>& buffers) {
    std::cout << "========================================================================================================\n";
    std::cout << " [도시 데이터 다중 비교 모드 (Compare Mode)]\n";
    std::cout << " 비교 대상: ";
    for (size_t i = 0; i < cityIds.size(); ++i) {
        std::cout << "ID " << (int)cityIds[i] << " " << GetCityName(cityIds[i]) << (i + 1 < cityIds.size() ? " vs " : "");
    }
    std::cout << "\n========================================================================================================\n";

    // 헤더 출력
    std::cout << std::left << std::setw(10) << "오프셋";
    for (uint8_t cid : cityIds) {
        std::string colName = "ID " + std::to_string(cid) + " (" + GetCityName(cid) + ")";
        std::cout << " | " << std::setw(24) << colName;
    }
    std::cout << " | 라벨\n";
    std::cout << "------------------------------------------------------------------------------------------------------------------------\n";

    size_t stride = CityMemoryLayout::OBJECT_STRIDE;
    for (size_t off = 0; off < stride; off += 4) {
        char offStr[16];
        std::snprintf(offStr, sizeof(offStr), "+0x%03X", (unsigned int)off);

        std::cout << std::left << std::setw(10) << offStr;

        for (size_t i = 0; i < cityIds.size(); ++i) {
            const auto& buf = buffers[i];
            uint32_t val32 = buf[off] | (buf[off + 1] << 8) | (buf[off + 2] << 16) | (buf[off + 3] << 24);
            uint16_t val16_0 = buf[off] | (buf[off + 1] << 8);
            uint16_t val16_1 = buf[off + 2] | (buf[off + 3] << 8);

            std::ostringstream oss;
            oss << val32 << " (" << val16_0 << "," << val16_1 << ")";
            std::cout << " | " << std::setw(24) << oss.str();
        }

        const char* label = GetKnownLabel(static_cast<uint16_t>(off));
        if (std::strlen(label) > 0) {
            std::cout << " | " << label;
        }
        std::cout << "\n";
    }
    std::cout << "========================================================================================================================\n\n";
}

// 특정 수치 검색 모드 (Find-Value 모드)
void FindValueInCities(uint32_t targetVal, const std::vector<uint8_t>& cityIds, const std::vector<std::vector<uint8_t>>& buffers) {
    std::cout << "========================================================================================\n";
    std::cout << " [특정 수치 검색 모드] 목표 검색 값: " << targetVal << " (Hex: 0x" << std::hex << targetVal << std::dec << ")\n";
    std::cout << "========================================================================================\n";

    size_t stride = CityMemoryLayout::OBJECT_STRIDE;
    int matchCount = 0;

    for (size_t i = 0; i < cityIds.size(); ++i) {
        uint8_t cid = cityIds[i];
        const auto& buf = buffers[i];

        // 1. uint32 일치 검색
        for (size_t off = 0; off + 3 < stride; off += 2) {
            uint32_t u32 = buf[off] | (buf[off + 1] << 8) | (buf[off + 2] << 16) | (buf[off + 3] << 24);
            if (u32 == targetVal) {
                std::cout << "  [일치-uint32] 도시 " << (int)cid << " (" << GetCityName(cid) << ")"
                          << " 오프셋: +0x" << std::hex << std::setw(3) << std::setfill('0') << off << std::dec << std::setfill(' ')
                          << " | 값: " << u32 << " " << GetKnownLabel(static_cast<uint16_t>(off)) << "\n";
                matchCount++;
            }
        }

        // 2. uint16 일치 검색 (targetVal이 65535 이하일 경우)
        if (targetVal <= 0xFFFF) {
            uint16_t target16 = static_cast<uint16_t>(targetVal);
            for (size_t off = 0; off + 1 < stride; off += 2) {
                uint16_t u16 = buf[off] | (buf[off + 1] << 8);
                if (u16 == target16) {
                    std::cout << "  [일치-uint16] 도시 " << (int)cid << " (" << GetCityName(cid) << ")"
                              << " 오프셋: +0x" << std::hex << std::setw(3) << std::setfill('0') << off << std::dec << std::setfill(' ')
                              << " | 값: " << u16 << " " << GetKnownLabel(static_cast<uint16_t>(off)) << "\n";
                    matchCount++;
                }
            }
        }
    }

    if (matchCount == 0) {
        std::cout << "  [-] 지정한 도시들의 368바이트 메모리 내에서 값 " << targetVal << "을(를) 찾지 못했습니다.\n";
    }
    std::cout << "========================================================================================\n\n";
}

void PrintUsage(const char* prog) {
    std::cout << "사용법:\n";
    std::cout << "  1. 단일 도시 상세 덤프:\n";
    std::cout << "     " << prog << " <도시 ID>                 (예: " << prog << " 18)\n\n";
    std::cout << "  2. 여러 도시 동시 비교 (Compare/Diff 모드):\n";
    std::cout << "     " << prog << " --compare <ID1> <ID2> ...    (예: " << prog << " --compare 0 9 18 20)\n\n";
    std::cout << "  3. 특정 수치 오프셋 역추적 검색 (Find-Value 모드):\n";
    std::cout << "     " << prog << " --find <수치> [도시 ID...]  (예: " << prog << " --find 3000 0 9 18)\n";
    std::cout << "     * 도시 ID 생략 시 주요 4대 도시(양평 0, 업 9, 낙양 18, 장안 20) 자동 검색\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] Error: 실행 중인 삼국지 13 PK 프로세스(San13PK_kr.exe)를 찾을 수 없습니다.\n";
        return 1;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc) || rootBase == 0) {
        std::cerr << "[-] Error: 루트 베이스 주소 산출 실패\n";
        return 1;
    }

    uintptr_t cityArrayBase = rootBase + CityMemoryLayout::CHUNK_OFFSET;
    size_t stride = CityMemoryLayout::OBJECT_STRIDE;

    std::string mode = argv[1];

    if (mode == "--compare") {
        if (argc < 4) {
            std::cerr << "[-] Error: --compare 모드는 최소 2개 이상의 도시 ID가 필요합니다.\n";
            std::cerr << "예시: " << argv[0] << " --compare 0 9 18 20\n";
            return 1;
        }
        std::vector<uint8_t> cityIds;
        std::vector<std::vector<uint8_t>> buffers;
        for (int i = 2; i < argc; ++i) {
            int cid = std::stoi(argv[i]);
            if (cid < 0 || cid >= CityMemoryLayout::MAX_CITY_COUNT) {
                std::cerr << "[-] Error: 유효하지 않은 도시 ID: " << cid << " (0~59)\n";
                return 1;
            }
            cityIds.push_back(static_cast<uint8_t>(cid));
            std::vector<uint8_t> buf(stride, 0);
            uintptr_t cityAddr = cityArrayBase + (cid * stride);
            driver.ReadRaw(cityAddr, buf.data(), stride);
            buffers.push_back(buf);
        }
        CompareCities(cityIds, buffers);
    } else if (mode == "--find") {
        if (argc < 3) {
            std::cerr << "[-] Error: --find 모드는 검색할 수치가 필요합니다.\n";
            std::cerr << "예시: " << argv[0] << " --find 3000\n";
            return 1;
        }
        uint32_t targetVal = static_cast<uint32_t>(std::stoul(argv[2]));
        std::vector<uint8_t> cityIds;
        if (argc >= 4) {
            for (int i = 3; i < argc; ++i) {
                int cid = std::stoi(argv[i]);
                if (cid >= 0 && cid < CityMemoryLayout::MAX_CITY_COUNT) {
                    cityIds.push_back(static_cast<uint8_t>(cid));
                }
            }
        } else {
            // 기본 주요 4대 표본 도시
            cityIds = {0, 9, 18, 20}; // 양평, 업, 낙양, 장안
        }
        std::vector<std::vector<uint8_t>> buffers;
        for (uint8_t cid : cityIds) {
            std::vector<uint8_t> buf(stride, 0);
            uintptr_t cityAddr = cityArrayBase + (cid * stride);
            driver.ReadRaw(cityAddr, buf.data(), stride);
            buffers.push_back(buf);
        }
        FindValueInCities(targetVal, cityIds, buffers);
    } else {
        int cid = std::stoi(argv[1]);
        if (cid < 0 || cid >= CityMemoryLayout::MAX_CITY_COUNT) {
            std::cerr << "[-] Error: 유효하지 않은 도시 ID: " << cid << " (0~59)\n";
            return 1;
        }
        std::vector<uint8_t> buf(stride, 0);
        uintptr_t cityAddr = cityArrayBase + (cid * stride);
        if (!driver.ReadRaw(cityAddr, buf.data(), stride)) {
            std::cerr << "[-] Error: 도시 메모리 읽기 실패 (주소: 0x" << std::hex << cityAddr << std::dec << ")\n";
            return 1;
        }
        DumpSingleCity(static_cast<uint8_t>(cid), buf.data(), stride);
    }

    return 0;
}
