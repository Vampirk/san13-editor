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

// 등록 신무장(editdata_pk.s13) 한글 이름 로더 (Phase 5.9-A)
// Steam Proton 경로 또는 지정된 파일에서 유저 생성 신무장 성명을 로드합니다.
size_t LoadCustomOfficerNames(const std::string& customFilePath = "");
void ClearCustomOfficerNames();

} // namespace San13::Game
