#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace San13::Game {

constexpr size_t HISTORICAL_OFFICER_NAME_COUNT = 700;

// 삼국지 13 PK 무장 ID에 대응하는 공식 한글 무장 이름 조회 (O(1))
// ID가 테이블에 없으면 빈 string_view ("")를 반환합니다.
std::string_view GetOfficerKoreanName(uint16_t id);

// 해당 ID의 무장 한글 이름이 존재하는지 확인
bool HasOfficerKoreanName(uint16_t id);

// 고대무장 슬롯 판별 (EDGE-56): 기본 고대무장 블록(850 ~ 899) + PK 특전 고대/왜국 무장(1251 ~ 1263)
// 출신 필터, 검색 키워드, [고대] 뱃지가 모두 이 함수를 기준으로 판별합니다.
constexpr bool IsAncientOfficerId(uint16_t id) {
    return (id >= 850 && id < 900) || (id >= 1251 && id <= 1263);
}

// 등록 신무장(editdata_pk.s13) 한글 이름 로더 (Phase 5.9-A)
// Steam Proton 경로 또는 지정된 파일에서 유저 생성 신무장 성명을 로드합니다.
size_t LoadCustomOfficerNames(const std::string& customFilePath = "");
void ClearCustomOfficerNames();

} // namespace San13::Game
