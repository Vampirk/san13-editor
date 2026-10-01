#pragma once

#include "core/IMemoryDriver.hpp"
#include <string>
#include <cstdint>

namespace San13::Game {

struct VersionEntry {
    uint32_t timedatestamp;
    uint32_t version_id;
    uint32_t global_offset;
    const char* description;
};

class GameVersionDetector {
public:
    /**
     * @brief 프로세스 메모리의 PE 헤더를 읽어 삼국지 13 PK 버전 및 RootBase 주소를 자동 산출
     * @param driver 메모리 드라이버
     * @param outRootBase 계산된 데이터 루트 주소 출력
     * @param outDesc 버전 설명 문자열 출력
     * @return 성공 여부
     */
    static bool DetectRootBase(Core::IMemoryDriver& driver, uintptr_t& outRootBase, std::string& outDesc);
};

} // namespace San13::Game
