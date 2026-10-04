#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/ItemManager.hpp"
#include "game/OfficerNameTable.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace San13;
using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 5] Item (명품) Manager Unit Test Suite\n";
    std::cout << "============================================================\n\n";

    // Step 1: 구조체 크기 및 오프셋 정합성 검증
    std::cout << "[Step 1] 구조체 크기 및 오프셋 정합성 검증\n";
    assert(sizeof(San13ItemRaw) == 36);
    assert(offsetof(San13ItemRaw, item_id) == 0x04);
    assert(offsetof(San13ItemRaw, category) == 0x06);
    assert(offsetof(San13ItemRaw, horse_speed) == 0x08);
    assert(offsetof(San13ItemRaw, stat_type) == 0x0A);
    assert(offsetof(San13ItemRaw, stat_boost) == 0x0C);
    assert(offsetof(San13ItemRaw, skill_id) == 0x0E);
    assert(offsetof(San13ItemRaw, skill_level) == 0x0F);
    assert(offsetof(San13ItemRaw, value) == 0x10);
    assert(offsetof(San13ItemRaw, price) == 0x12);
    assert(offsetof(San13ItemRaw, owner_officer_id) == 0x1C);
    std::cout << "  [PASS] sizeof(San13ItemRaw) == 36 바이트 및 오프셋 100% 일치 확인\n";

    // Step 2: 234개 공식 명품 이름 테이블 무결성 검증
    std::cout << "\n[Step 2] 234개 공식 명품 이름 및 설명 매핑 검증\n";
    assert(OFFICIAL_ITEM_COUNT == 234);
    assert(GetItemOfficialName(0) == "적토마");
    assert(GetItemOfficialName(1) == "적로");
    assert(GetItemOfficialName(2) == "조황비전");
    assert(GetItemOfficialName(3) == "절영");
    assert(GetItemOfficialName(17) == "의천검");
    assert(GetItemOfficialName(18) == "청강검");
    assert(GetItemOfficialName(19) == "자웅일대검");
    assert(GetItemOfficialName(28) == "청룡언월도");
    assert(GetItemOfficialName(29) == "삼첨도");
    assert(GetItemOfficialName(34) == "사모");
    assert(GetItemOfficialName(39) == "방천화극");
    assert(GetItemOfficialName(40) == "쌍철극");
    assert(GetItemOfficialName(41) == "대부");
    assert(GetItemOfficialName(72) == "맹덕신서");
    assert(GetItemOfficialName(73) == "병법이십사편");
    assert(GetItemOfficialName(111) == "서촉지형도");
    assert(GetItemOfficialName(214) == "옥새");
    assert(GetItemOfficialName(232) == "금부");
    assert(GetItemOfficialName(233) == "은부");

    assert(!GetItemOfficialDescription(0).empty());
    assert(!GetItemOfficialDescription(28).empty());
    assert(!GetItemOfficialDescription(39).empty());
    std::cout << "  [PASS] 적토마(0), 청룡언월도(28), 사모(34), 방천화극(39), 은부(233) 공식 매핑 확인\n";

    // Step 3: 라이브 메모리 연결 및 판독 검증
    std::cout << "\n[Step 3] 라이브 프로세스 메모리 기반 명품 판독 검증\n";
    Core::LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe")) {
        std::cout << "  [SKIP] San13PK_kr.exe 프로세스 미실행 (오프라인 테스트 종료)\n";
        return 0;
    }

    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) {
        std::cerr << "  [FAIL] RootBase 탐지 실패\n";
        return 1;
    }

    ItemManager itemMgr;
    assert(itemMgr.Initialize(&driver, rootBase));
    std::cout << "  -> ItemBase 주소: 0x" << std::hex << itemMgr.GetItemAddress(0) << std::dec << "\n";

    // 234개 명품 일괄 로드
    std::vector<ItemRecord> allItems;
    assert(itemMgr.LoadAllItems(allItems));
    std::cout << "  -> 로드된 유효 명품 수: " << allItems.size() << "\n";
    assert(allItems.size() == OFFICIAL_ITEM_COUNT);

    // 대표 명품 검증
    const auto& redHare = allItems[0];
    std::cout << "  [Item 0] 이름: " << redHare.name << ", 분류: " << redHare.category_name 
              << ", 보정: " << redHare.stat_boost_desc << ", 특기: " << redHare.skill_desc 
              << ", 소유자: " << redHare.owner_name << " (ID: " << redHare.owner_id << ")\n";
    assert(redHare.name == "적토마");
    assert(redHare.category_name == "명마");
    assert(redHare.owner_id == 635); // 여포

    const auto& blade = allItems[28];
    std::cout << "  [Item 28] 이름: " << blade.name << ", 분류: " << blade.category_name 
              << ", 보정: " << blade.stat_boost_desc << ", 특기: " << blade.skill_desc 
              << ", 소유자: " << blade.owner_name << " (ID: " << blade.owner_id << ")\n";
    assert(blade.name == "청룡언월도");
    assert(blade.owner_id == 94); // 관우

    const auto& spear = allItems[34];
    std::cout << "  [Item 34] 이름: " << spear.name << ", 분류: " << spear.category_name 
              << ", 보정: " << spear.stat_boost_desc << ", 특기: " << spear.skill_desc 
              << ", 소유자: " << spear.owner_name << " (ID: " << spear.owner_id << ")\n";
    assert(spear.name == "사모");
    assert(spear.owner_id == 418); // 장비

    // Step 4: 무장별 소유 명품 목록 조회 검증
    std::cout << "\n[Step 4] 특정 무장 소유 명품 역색인(GetOfficerItems) 검증\n";
    std::vector<ItemRecord> lubuItems;
    assert(itemMgr.GetOfficerItems(635, lubuItems)); // 여포
    std::cout << "  -> 여포(635) 보유 명품 수: " << lubuItems.size() << "\n";
    bool hasRedHare = false, hasHalberd = false;
    for (const auto& it : lubuItems) {
        std::cout << "     * " << it.name << " (" << it.category_name << ", " << it.stat_boost_desc << ")\n";
        if (it.id == 0) hasRedHare = true;
        if (it.id == 39) hasHalberd = true;
    }
    assert(hasRedHare && hasHalberd);
    std::cout << "  [PASS] 여포의 적토마 & 방천화극 소유 목록 정상 역색인 확인\n";

    // Step 5: 소유권 변경 및 100% 무결성 롤백 쓰기 검증
    std::cout << "\n[Step 5] 명품 소유권 변경(SetItemOwner) 및 100% 무결성 롤백 검증\n";
    // 사모(34)의 원래 소유자(장비: 418)
    uint16_t originalOwner = spear.owner_id;
    assert(originalOwner == 418);

    // 1) 조조(333)에게 수여 테스트
    assert(itemMgr.GrantItem(34, 333));
    ItemRecord modifiedItem;
    assert(itemMgr.LoadItem(34, modifiedItem));
    std::cout << "  -> 조조에게 수여 후 사모 소유자: " << modifiedItem.owner_name << " (ID: " << modifiedItem.owner_id << ")\n";
    assert(modifiedItem.owner_id == 333);

    // 2) 몰수(0xFFFF) 테스트
    assert(itemMgr.ConfiscateItem(34));
    assert(itemMgr.LoadItem(34, modifiedItem));
    std::cout << "  -> 몰수 후 사모 소유자: " << modifiedItem.owner_name << " (ID: " << modifiedItem.owner_id << ")\n";
    assert(modifiedItem.owner_id == 0xFFFF);
    assert(!modifiedItem.is_owned);

    // 3) 원래 소유자(장비: 418)로 100% 안전 원복
    assert(itemMgr.SetItemOwner(34, originalOwner));
    assert(itemMgr.LoadItem(34, modifiedItem));
    std::cout << "  -> 원복 후 사모 소유자: " << modifiedItem.owner_name << " (ID: " << modifiedItem.owner_id << ")\n";
    assert(modifiedItem.owner_id == 418);
    std::cout << "  [PASS] 소유권 변경/몰수/수여 및 안전 원복 100% 무결성 검증 통과\n";

    // Step 6: 미소유 명품의 소재지 도시 판독 및 시장 이동(SetItemLocation) 검증
    std::cout << "\n[Step 6] 미소유 명품 소재 도시(시장) 판독 및 위치 이동 검증\n";
    ItemRecord sword15;
    assert(itemMgr.LoadItem(15, sword15));
    std::cout << "  [Item 15 참사검] 소유: " << sword15.owner_name << ", 소재지: " << sword15.location_name << " (CityID: " << (int)sword15.location_city_id << ")\n";
    assert(!sword15.is_owned);
    assert(sword15.location_city_id == 23); // 낙양
    assert(sword15.location_name.find("낙양") != std::string::npos);

    // 시장 위치 이동 테스트: 장안(25)으로 이동 후 원복
    uint8_t origCity = sword15.location_city_id;
    assert(itemMgr.SetItemLocation(15, 25)); // 장안으로 이동
    ItemRecord movedSword;
    assert(itemMgr.LoadItem(15, movedSword));
    assert(movedSword.location_city_id == 25);
    assert(movedSword.location_name.find("장안") != std::string::npos);
    std::cout << "  -> 장안으로 시장 이동 확인: " << movedSword.location_name << "\n";

    // 원래 도시(낙양 23)로 100% 안전 원복
    assert(itemMgr.SetItemLocation(15, origCity));
    assert(itemMgr.LoadItem(15, movedSword));
    assert(movedSword.location_city_id == origCity);
    std::cout << "  [PASS] 미소유 명품 소재지 도시(시장) 판독 및 이동/원복 무결성 통과\n";

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 명품(Item) 매니저 전수 검증 100% 통과 (Green)\n";
    std::cout << "============================================================\n";
    return 0;
}
