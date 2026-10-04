#include "game/TacticManager.hpp"
#include "core/IMemoryDriver.hpp"
#include <fstream>
#include <vector>
#include <array>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <iconv.h>

namespace San13::Game {

static const char* const VANILLA_TACTIC_NAMES[VANILLA_TACTIC_COUNT] = {
    "천하무쌍", "괴력무쌍", "군신", "만인적", "소패왕", "사자분신",
    "투장용약", "투지호장", "고금무쌍", "국사무쌍", "천궁", "신궁",
    "인왕서기", "투지용장", "불굴강장", "불굴용장", "부동", "불굴",
    "특공", "철벽", "방원진", "역경강화", "연격", "기병돌격",
    "연사", "강궁", "공격강화", "방어강화", "기동강화", "창공강화",
    "창방강화", "창속강화", "기공강화", "기방강화", "기속강화", "궁공강화",
    "궁방강화", "궁속강화", "주위공격", "사기회복", "결사지휘", "견수지휘",
    "대의지휘", "천의무봉", "용략지휘", "공수지휘", "맹장돌격", "열장돌격",
    "효장질구", "신산진법", "시황제", "위풍지휘", "돌진지휘", "돌격지휘",
    "질주지휘", "특공지휘", "철벽지휘", "속방지휘", "역경지휘", "맹사지휘",
    "제사지휘", "공격지휘", "수비지휘", "속공지휘", "공격명령", "수비명령",
    "속공명령", "창병공격진", "창병수비진", "창병속공진", "기병공격진", "기병수비진",
    "기병속공진", "궁병공격진", "궁병수비진", "궁병속공진", "고무", "위무",
    "왕좌", "군략지원", "후방지원", "신의", "전진강화", "위병",
    "허보", "귀모계", "연환계", "종횡기략", "요염", "이간계",
    "동요", "선동", "교란", "위보", "위보동요", "위보선동",
    "위보교란", "도발", "동요도발", "선동도발", "교란도발", "창병동요",
    "기병동요", "궁병동요", "창병선동", "기병선동", "궁병선동", "소침공작",
    "공격약화", "수비약화", "기동약화", "창공약화", "창방약화", "창속약화",
    "기공약화", "기방약화", "기속약화", "궁공약화", "궁방약화", "궁속약화",
    "창병도발", "기병도발", "궁병도발", "열화계", "신화계", "화우계",
    "육도비계", "태평요술", "요술", "대화계", "화공계", "화염계",
    "화계", "흉노난사", "오환맹사", "산월역경", "남만특공", "저강철벽",
    "단일도발", "대도발", "금족"
};

static std::array<CustomTacticEntry, CUSTOM_TACTIC_MAX_COUNT> s_customTactics;
static std::vector<std::pair<uint16_t, std::string>> s_availableTactics;
static bool s_initialized = false;

static std::string Cp949ToUtf8(const char* data, size_t maxLen) {
    if (!data) return "";
    size_t inLen = 0;
    while (inLen < maxLen && data[inLen] != '\0') inLen++;
    if (inLen == 0) return "";

    iconv_t cd = iconv_open("UTF-8", "CP949");
    if (cd == (iconv_t)-1) return std::string(data, inLen);

    size_t inbytes = inLen;
    size_t outbytes = inLen * 3 + 1;
    std::vector<char> outbuf(outbytes);
    char* inptr = const_cast<char*>(data);
    char* outptr = outbuf.data();

    if (iconv(cd, &inptr, &inbytes, &outptr, &outbytes) == (size_t)-1) {
        iconv_close(cd);
        return std::string(data, inLen);
    }
    iconv_close(cd);
    return std::string(outbuf.data(), outptr - outbuf.data());
}

static std::string FindEditDataPath(const std::string& customFilePath) {
    if (!customFilePath.empty() && access(customFilePath.c_str(), R_OK) == 0) {
        return customFilePath;
    }

    const char* home = std::getenv("HOME");
    if (!home) return "";

    std::vector<std::string> candidates = {
        std::string(home) + "/.local/share/Steam/steamapps/compatdata/363150/pfx/drive_c/users/steamuser/Documents/KoeiTecmo/San13/KR_SAVEDATA/editdata_pk.s13",
        std::string(home) + "/.steam/steam/steamapps/compatdata/363150/pfx/drive_c/users/steamuser/Documents/KoeiTecmo/San13/KR_SAVEDATA/editdata_pk.s13",
        std::string(home) + "/Documents/KoeiTecmo/San13/KR_SAVEDATA/editdata_pk.s13"
    };

    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        candidates.push_back(std::string(userProfile) + "/Documents/KoeiTecmo/San13/KR_SAVEDATA/editdata_pk.s13");
    }

    for (const auto& p : candidates) {
        if (access(p.c_str(), R_OK) == 0) return p;
    }
    return "";
}

static void RebuildAvailableTactics() {
    s_availableTactics.clear();
    s_availableTactics.reserve(VANILLA_TACTIC_COUNT + CUSTOM_TACTIC_MAX_COUNT);

    for (size_t i = 0; i < VANILLA_TACTIC_COUNT; ++i) {
        s_availableTactics.emplace_back(static_cast<uint16_t>(i), VANILLA_TACTIC_NAMES[i]);
    }

    for (size_t i = 0; i < CUSTOM_TACTIC_MAX_COUNT; ++i) {
        if (s_customTactics[i].is_valid && !s_customTactics[i].name.empty()) {
            s_availableTactics.emplace_back(s_customTactics[i].id, s_customTactics[i].name);
        }
    }
}

const char* TacticManager::GetTacticName(uint16_t tacticId) {
    if (tacticId < VANILLA_TACTIC_COUNT) {
        return VANILLA_TACTIC_NAMES[tacticId];
    }

    if (tacticId >= CUSTOM_TACTIC_START_ID && tacticId < CUSTOM_TACTIC_START_ID + CUSTOM_TACTIC_MAX_COUNT) {
        size_t idx = tacticId - CUSTOM_TACTIC_START_ID;
        if (!s_initialized) {
            LoadCustomTactics();
        }
        if (s_customTactics[idx].is_valid && !s_customTactics[idx].name.empty()) {
            return s_customTactics[idx].name.c_str();
        }
        static char fallback[32];
        std::snprintf(fallback, sizeof(fallback), "등록 전법 %u", tacticId);
        return fallback;
    }

    return "미상 전법";
}

size_t TacticManager::LoadCustomTactics(const std::string& customFilePath) {
    std::string path = FindEditDataPath(customFilePath);
    if (path.empty()) return 0;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return 0;

    // editdata_pk.s13 EntryTactics 블록
    // 오프셋: 0x122FE0 + 32 (0x123000)
    // 스트라이드: 117 바이트 (0x75)
    constexpr size_t ENTRY_TACTICS_OFFSET = 0x122FE0 + 32;
    constexpr size_t TACTIC_STRIDE = 117;

    file.seekg(ENTRY_TACTICS_OFFSET, std::ios::beg);
    if (!file.good()) return 0;

    size_t loadedCount = 0;
    std::vector<char> record(TACTIC_STRIDE);

    for (size_t i = 0; i < CUSTOM_TACTIC_MAX_COUNT; ++i) {
        if (!file.read(record.data(), TACTIC_STRIDE)) break;

        uint16_t flag = *reinterpret_cast<uint16_t*>(&record[0]);
        uint16_t id = CUSTOM_TACTIC_START_ID + static_cast<uint16_t>(i);

        s_customTactics[i].id = id;
        if (flag != 0xFFFF) {
            // 유효한 등록 전법
            s_customTactics[i].is_valid = true;
            s_customTactics[i].name = Cp949ToUtf8(&record[2], 32);
            s_customTactics[i].desc = Cp949ToUtf8(&record[42], 64);
            loadedCount++;
        } else {
            s_customTactics[i].is_valid = false;
            s_customTactics[i].name.clear();
            s_customTactics[i].desc.clear();
        }
    }

    s_initialized = true;
    RebuildAvailableTactics();
    return loadedCount;
}

size_t TacticManager::LoadCustomTacticsFromMemory(Core::IMemoryDriver* driver, uintptr_t rootBase) {
    if (!driver || !driver->IsAlive() || rootBase == 0) return 0;

    // 인게임 프로세스 메모리 전법 청크
    // 오프셋: RootBase - 0x131640
    // 스트라이드: 124 바이트 (0x7C)
    // 구조: +0x00 vtable, +0x04 id, +0x06 flag, +0x08 name (32B CP949), +0x28 desc
    constexpr uintptr_t LIVE_TACTIC_OFFSET_FROM_ROOT = 0x131640;
    constexpr size_t LIVE_STRIDE = 124;

    uintptr_t tableBase = rootBase - LIVE_TACTIC_OFFSET_FROM_ROOT;

    size_t loadedCount = 0;
    std::vector<uint8_t> buffer(LIVE_STRIDE);

    for (size_t i = 0; i < CUSTOM_TACTIC_MAX_COUNT; ++i) {
        uintptr_t entryAddr = tableBase + (i * LIVE_STRIDE);
        if (!driver->ReadRaw(entryAddr, buffer.data(), LIVE_STRIDE)) continue;

        uint16_t id = *reinterpret_cast<uint16_t*>(&buffer[4]);
        int16_t flag = *reinterpret_cast<int16_t*>(&buffer[6]);

        s_customTactics[i].id = id;
        if (flag != -1 && flag != 0) {
            s_customTactics[i].is_valid = true;
            s_customTactics[i].name = Cp949ToUtf8(reinterpret_cast<const char*>(&buffer[8]), 32);
            s_customTactics[i].desc = Cp949ToUtf8(reinterpret_cast<const char*>(&buffer[40]), 64);
            loadedCount++;
        } else {
            // 메모리에 아직 로드되지 않은 경우 기존 파일 캐시 유지
            if (!s_customTactics[i].is_valid) {
                s_customTactics[i].name.clear();
                s_customTactics[i].desc.clear();
            }
        }
    }

    s_initialized = true;
    RebuildAvailableTactics();
    return loadedCount;
}

const std::vector<std::pair<uint16_t, std::string>>& TacticManager::GetAvailableTactics() {
    if (!s_initialized) {
        LoadCustomTactics();
    }
    return s_availableTactics;
}

const CustomTacticEntry* TacticManager::GetCustomTactic(uint16_t tacticId) {
    if (tacticId >= CUSTOM_TACTIC_START_ID && tacticId < CUSTOM_TACTIC_START_ID + CUSTOM_TACTIC_MAX_COUNT) {
        size_t idx = tacticId - CUSTOM_TACTIC_START_ID;
        if (!s_initialized) LoadCustomTactics();
        if (s_customTactics[idx].is_valid) {
            return &s_customTactics[idx];
        }
    }
    return nullptr;
}

void TacticManager::Clear() {
    for (auto& entry : s_customTactics) {
        entry.is_valid = false;
        entry.name.clear();
        entry.desc.clear();
    }
    s_availableTactics.clear();
    s_initialized = false;
}

} // namespace San13::Game
