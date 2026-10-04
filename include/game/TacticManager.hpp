#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace San13::Core {
class IMemoryDriver;
}

namespace San13::Game {

constexpr size_t VANILLA_TACTIC_COUNT = 141;
constexpr uint16_t CUSTOM_TACTIC_START_ID = 250;
constexpr size_t CUSTOM_TACTIC_MAX_COUNT = 50; // 250 ~ 299

struct CustomTacticEntry {
    uint16_t id = 0;              // 250 ~ 299
    bool is_valid = false;        // 유효 등록 여부
    std::string name;             // 전법 명칭 (UTF-8)
    std::string desc;             // 전법 설명 (UTF-8)
    uint8_t command_points = 0;   // 소모 지휘 포인트
};

class TacticManager {
public:
    // 전법명 조회 (0~140: 바닐라 전법, 250~299: 등록 전법, 미등록/미상은 대체 텍스트)
    static const char* GetTacticName(uint16_t tacticId);

    // editdata_pk.s13 파일로부터 등록 전법 로드
    static size_t LoadCustomTactics(const std::string& customFilePath = "");

    // 실시간 프로세스 메모리로부터 등록 전법 로드 (RootBase - 0x131640)
    static size_t LoadCustomTacticsFromMemory(Core::IMemoryDriver* driver, uintptr_t rootBase);

    // 콤보박스 및 선택용 사용 가능 전법 목록 (바닐라 141개 + 실제 등록된 전법)
    static const std::vector<std::pair<uint16_t, std::string>>& GetAvailableTactics();

    // 특정 등록 전법 상세 정보 조회
    static const CustomTacticEntry* GetCustomTactic(uint16_t tacticId);

    // 캐시 초기화
    static void Clear();
};

} // namespace San13::Game
