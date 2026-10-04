#pragma once

#include <cstdint>
#include <string_view>
#include <string>

namespace San13::Game {

constexpr size_t OFFICIAL_ITEM_COUNT = 234;

// 공식 명품 한글 이름 및 설명 조회 API
std::string_view GetItemOfficialName(uint16_t itemId);
std::string_view GetItemOfficialDescription(uint16_t itemId);
bool HasItemOfficialName(uint16_t itemId);

// 카테고리 한국어 명칭 반환 (0: 명마, 1: 검, 2: 도, 3: 대도, 4: 창/모, 5: 극, 6: 부, 7: 편, 8: 추, 9: 암기, 10: 궁, 11: 병서, 12: 경서, 13: 사서, 14: 자서, 15: 문집, 16: 의서, 17: 정서, 18: 지도, 19: 보물, 20: 주류, 21: 약재 등)
std::string_view GetItemCategoryName(uint8_t category);

// 능력치 종류 한국어 명칭 반환 (1: 무력, 2: 통솔, 3: 지력, 4: 정치)
std::string_view GetItemStatTypeName(uint8_t statType);

} // namespace San13::Game
