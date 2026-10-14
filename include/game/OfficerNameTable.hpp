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

// 등록 신무장(editdata_pk.s13) 성명/열전 로더 (Phase 5.9-A, BUG-02)
// Steam Proton 경로 또는 지정된 파일에서 1,000개 레코드를 파싱합니다 (레코드 0~149 → ID 700~849, 150~999 → ID 1315~2164).
// 반환값: 성명이 있는(사용 중인) 레코드 수
size_t LoadCustomOfficerNames(const std::string& customFilePath = "");

// 등록 신무장 열전 조회 (BUG-02). 등록 신무장 슬롯이 아니거나 열전이 없으면 빈 string_view
std::string_view GetCustomOfficerBiography(uint16_t id);
void ClearCustomOfficerNames();

} // namespace San13::Game
