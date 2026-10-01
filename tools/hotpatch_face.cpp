#include <iostream>
#include <iomanip>
#include <string>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"

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

    uint16_t target_id = 333; // 기본 대상: 조조 (333)
    uint16_t new_face = 117;   // 변경할 얼굴: 관우 (117)

    if (argc >= 2) target_id = std::stoi(argv[1]);
    if (argc >= 3) new_face = std::stoi(argv[2]);

    uintptr_t objAddr = POOL + target_id * STRIDE;
    uint16_t cur_face = 0;
    driver.ReadRaw(objAddr + 0x08, &cur_face, sizeof(cur_face));

    std::cout << "============================================================\n";
    std::cout << " [Step 6.0-3] Face ID (+0x08) 실시간 인게임 핫패치 도구\n";
    std::cout << "============================================================\n";
    std::cout << " 대상 무장 ID: " << target_id << " (조조=333, 항적=857)\n";
    std::cout << " 현재 Face ID (+0x08): " << cur_face << "\n";
    std::cout << " 변경 목표 Face ID  : " << new_face << " (관우=117, 여포=786, 제갈량=347)\n";

    // 메모리 쓰기 수행
    if (driver.WriteRaw(objAddr + 0x08, &new_face, sizeof(new_face))) {
        uint16_t verify_face = 0;
        driver.ReadRaw(objAddr + 0x08, &verify_face, sizeof(verify_face));
        if (verify_face == new_face) {
            std::cout << " [\033[32m성공\033[0m] 메모리 쓰기 완료! 검증값: " << verify_face << "\n";
            std::cout << " >>> 인게임에서 해당 무장의 정보창을 열어 얼굴 일러스트가 바뀌었는지 확인하세요! <<<\n";
        } else {
            std::cerr << " [-] 검증 실패 (읽은 값: " << verify_face << ")\n";
        }
    } else {
        std::cerr << " [-] 메모리 쓰기 실패\n";
    }

    return 0;
}
