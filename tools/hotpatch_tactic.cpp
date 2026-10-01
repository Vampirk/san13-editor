#include <iostream>
#include <iomanip>
#include <string>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"
#include "game/OfficerNameTable.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main(int argc, char* argv[]) {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) {
        std::cerr << "[-] 게임 프로세스 연결 실패\n";
        return 1;
    }

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    const uintptr_t POOL = root_base + OfficerMemoryLayout::POOL_OFFSET;
    const size_t STRIDE = OfficerMemoryLayout::OBJECT_STRIDE;

    uint16_t target_id = 857; // 기본 대상: 항적 (857)
    uint8_t new_tactic = 0;   // 변경 목표 전법: 0 (천하무쌍)

    if (argc >= 2) target_id = std::stoi(argv[1]);
    if (argc >= 3) new_tactic = std::stoi(argv[2]);

    uintptr_t objAddr = POOL + target_id * STRIDE;
    uint8_t cur_tactic = 0;
    driver.ReadRaw(objAddr + 0x24, &cur_tactic, sizeof(cur_tactic));

    std::string_view officer_name = GetOfficerKoreanName(target_id);
    if (officer_name.empty()) {
        officer_name = (target_id == 857) ? "항적" : "무장";
    }

    std::cout << "============================================================\n";
    std::cout << " [Step 6.0-2] 전법 ID (+0x24) 실시간 인게임 핫패치 도구\n";
    std::cout << "============================================================\n";
    std::cout << " 대상 무장: " << officer_name << " (ID " << target_id << ")\n";
    std::cout << " 현재 전법: " << (int)cur_tactic << " (" << OfficerRecord::GetTacticName(cur_tactic) << ")\n";
    std::cout << " 변경 목표: " << (int)new_tactic << " (" << OfficerRecord::GetTacticName(new_tactic) << ")\n";

    // 메모리 쓰기 수행
    if (driver.WriteRaw(objAddr + 0x24, &new_tactic, sizeof(new_tactic))) {
        uint8_t verify_tactic = 0;
        driver.ReadRaw(objAddr + 0x24, &verify_tactic, sizeof(verify_tactic));
        if (verify_tactic == new_tactic) {
            std::cout << " [\033[32m성공\033[0m] 메모리 쓰기 완료! 검증값: " << (int)verify_tactic 
                      << " (" << OfficerRecord::GetTacticName(verify_tactic) << ")\n";
            std::cout << " >>> 인게임에서 무장 정보창을 열어 보유 전법이 [" 
                      << OfficerRecord::GetTacticName(verify_tactic) << "](으)로 변경되었는지 확인하세요! <<<\n";
        } else {
            std::cerr << " [-] 검증 실패 (읽은 값: " << (int)verify_tactic << ")\n";
        }
    } else {
        std::cerr << " [-] 메모리 쓰기 실패\n";
    }

    return 0;
}
