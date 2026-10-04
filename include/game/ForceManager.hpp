#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace San13::Core {
class IMemoryDriver;
}

namespace San13::Game {

struct ForceMemoryLayout {
    static constexpr uintptr_t CHUNK_OFFSET = 0x00ED60; // RootBase로부터의 세력 청크 시작 오프셋
    static constexpr size_t    OBJECT_STRIDE = 0x154;   // 340 바이트 Stride
    static constexpr size_t    RULER_OFFSET = 0x36;     // 군주 무장 ID 오프셋 (+0x36, uint16_t)
    static constexpr size_t    MAX_FORCE_COUNT = 64;    // 최대 64개 세력
};

struct ForceRecord {
    uint16_t force_id = 0xFFFF;
    uint16_t ruler_id = 0xFFFF;
    std::string ruler_name;
    std::string force_name;
    bool is_valid = false;
};

class ForceManager {
public:
    static bool Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase);
    static bool Refresh();

    // 세력 고유 명칭 조회 (예: "조조군", "원소군", "무소속")
    static std::string GetForceName(uint16_t forceId);

    // 군주 무장 ID 조회 (없으면 0xFFFF)
    static uint16_t GetForceRulerId(uint16_t forceId);

    // 유효한 세력인지 확인
    static bool IsValidForce(uint16_t forceId);

    // 전체 세력 목록 캐시 반환
    static const std::vector<ForceRecord>& GetForces();

private:
    static Core::IMemoryDriver* s_driver;
    static uintptr_t s_forceArrayBase;
    static std::vector<ForceRecord> s_forces;
    static bool s_initialized;
};

} // namespace San13::Game
