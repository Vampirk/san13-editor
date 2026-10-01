#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdint>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"
#include "game/OfficerNameTable.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "프로세스 연결 실패\n";
        return 1;
    }

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);
    std::cout << "RootBase: 0x" << std::hex << root_base << " (" << verDesc << ")\n";

    const uintptr_t POOL = root_base + OfficerMemoryLayout::POOL_OFFSET;
    const size_t STRIDE = OfficerMemoryLayout::OBJECT_STRIDE;

    std::cout << "\n=== [활성 무장 중 무력 90 이상 또는 특수 슬롯 검색] ===\n";
    for (uint16_t id = 0; id < OfficerMemoryLayout::MAX_TOTAL_SLOTS; ++id) {
        uintptr_t addr = POOL + id * STRIDE;
        San13OfficerHeader header{};
        if (!driver.ReadRaw(addr, &header, sizeof(header))) continue;

        if (header.activity_flag == 0xFF) continue; // 미할당 슬롯 건너뜀

        // 무력이 95 이상이거나, ID가 800 이상(고대/신무장)이면서 활동 중인 경우
        if (header.stats.base_war >= 95 || (id >= 700 && header.activity_flag > 0)) {
            std::string_view name = GetOfficerKoreanName(id);
            std::cout << "ID " << std::setw(4) << id 
                      << " | 이름: " << (name.empty() ? "(미등록/신무장)" : std::string(name))
                      << " | 통솔: " << (int)header.stats.base_leadership
                      << " 무력: " << (int)header.stats.base_war
                      << " 지력: " << (int)header.stats.base_intel
                      << " 정치: " << (int)header.stats.base_politics
                      << " | FaceID(+0x08): " << header.face_id
                      << " | 소속세력(+0x3C): " << header.force_id
                      << " | 상태(+0x4F): 0x" << std::hex << (int)header.activity_flag
                      << " | 성격(+0x4E): " << std::dec << (int)header.personality
                      << "\n";

            // +0x26 ~ +0x39 특기 20개 레벨 덤프
            std::cout << "   -> 특기[20]: ";
            for (int k = 0; k < 20; ++k) {
                std::cout << (int)header.skills[k] << " ";
            }
            std::cout << "\n";

            // +0x50 ~ +0x5F 첫 16바이트 덤프
            uint8_t extra[16] = {0};
            driver.ReadRaw(addr + 0x50, extra, 16);
            std::cout << "   -> +0x50[16]: ";
            for (int k = 0; k < 16; ++k) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)extra[k] << " ";
            }
            std::cout << std::dec << "\n";
        }
    }

    return 0;
}
