#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/Officer.hpp"

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;

    uintptr_t root_base = 0;
    std::string verDesc;
    GameVersionDetector::DetectRootBase(driver, root_base, verDesc);

    const uintptr_t POOL = root_base + OfficerMemoryLayout::POOL_OFFSET;
    const size_t STRIDE = OfficerMemoryLayout::OBJECT_STRIDE;

    // 공식 전법 목록 (상위 일부)
    static const char* const TACTIC_NAMES[] = {
        "천하무쌍", "괴력무쌍", "군신", "만인적", "소패왕", "사자분신",
        "투장용약", "투지호장", "고금무쌍", "국사무쌍", "천궁", "신궁",
        "인왕서기", "투지용장", "불굴강장", "불굴용장", "부동", "불굴",
        "특공", "철벽", "방원진", "역경강화", "연격", "기병돌격",
        "연사", "강궁", "공격강화", "방어강화", "기동강화", "창공강화",
        "창방강화", "창속강화", "기공강화", "기방강화", "기속강화", "궁공강화",
        "궁방강화", "궁속강화", "주위공격", "사기회복", "결사지휘", "견수지휘",
        "대의지휘", "천의무봉", "용략지휘", "공수지휘", "맹장돌격", "열장돌격",
        "효장질구", "신산진법", "시황제", "위풍지휘", "돌진지휘", "돌격지휘"
    };

    struct TestOfficer {
        uint16_t id;
        std::string name;
    };

    std::vector<TestOfficer> officers = {
        {635, "여포"},
        {94,  "관우"},
        {418, "장비"},
        {359, "손책"},
        {380, "조운"},
        {857, "항적"},
        {333, "조조"},
        {610, "유비"},
        {283, "제갈량"},
        {851, "범증"},
        {858, "우희"},
        {18,  "원소"}
    };

    std::cout << "============================================================\n";
    std::cout << " [대발굴 검증] 무장별 +0x24 (전법 ID) 실측 대조표\n";
    std::cout << "============================================================\n";

    for (const auto& off : officers) {
        uintptr_t addr = POOL + off.id * STRIDE;
        uint8_t tactic_id = 0;
        driver.ReadRaw(addr + 0x24, &tactic_id, 1);

        std::string tactic_name = (tactic_id < sizeof(TACTIC_NAMES)/sizeof(TACTIC_NAMES[0])) 
                                  ? TACTIC_NAMES[tactic_id] : "(ID " + std::to_string(tactic_id) + ")";

        std::cout << "무장: " << std::setw(8) << off.name 
                  << " (ID " << std::setw(3) << off.id << ")"
                  << " | +0x24 (전법 ID) = " << std::setw(3) << (int)tactic_id
                  << " -> 전법명: " << tactic_name << "\n";
    }

    return 0;
}
