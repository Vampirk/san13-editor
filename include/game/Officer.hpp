#pragma once

#include <cstdint>
#include <string>

namespace San13::Game {

#pragma pack(push, 1)

/**
 * @brief 활성 무장 오브젝트 내부의 16바이트 능력치/적성 블록 (오프셋 +0x15)
 */
struct San13OfficerLiveStats {
    uint8_t  base_leadership;    // +0x00 (+0x15): 기본 통솔 (아이템 제외 순수치)
    uint8_t  base_war;           // +0x01 (+0x16): 기본 무력
    uint8_t  base_intel;         // +0x02 (+0x17): 기본 지력
    uint8_t  base_politics;      // +0x03 (+0x18): 기본 정치
    uint8_t  curr_leadership;    // +0x04 (+0x19): 현재 통솔 (버프/아이템 반영)
    uint8_t  curr_war;           // +0x05 (+0x1A): 현재 무력
    uint8_t  curr_intel;         // +0x06 (+0x1B): 현재 지력
    uint8_t  curr_politics;      // +0x07 (+0x1C): 현재 정치
    uint8_t  exp_leadership;     // +0x08 (+0x1D): 통솔 경험치 (0 ~ 99)
    uint8_t  exp_war;            // +0x09 (+0x1E): 무력 경험치
    uint8_t  exp_intel;          // +0x0A (+0x1F): 지력 경험치
    uint8_t  exp_politics;       // +0x0B (+0x20): 정치 경험치
    uint8_t  spear_apt;          // +0x0C (+0x21): 창병 적성 (0:C, 1:B, 2:A, 3:S)
    uint8_t  horse_apt;          // +0x0D (+0x22): 기병 적성
    uint8_t  bow_apt;            // +0x0E (+0x23): 궁병 적성
    uint8_t  tactic_id;          // +0x0F (+0x24): 보유 전법 ID (0~140, Step 6.0-2 규명)
};

/**
 * @brief 무장 오브젝트 헤더부 (오프셋 +0x00 ~ +0x4F, 실측 검증 완료)
 */
struct San13OfficerHeader {
    uint32_t vtable;             // +0x00: VTable 포인터 (0x7291c510)
    uint32_t officer_id;         // +0x04: 고유 무장 ID
    uint16_t face_id;            // +0x08: 얼굴 CG 번호 (Face ID, Phase 6.0 실측 규명)
    uint16_t city_id;            // +0x0A: 소재 도시 ID
    uint8_t  rank;               // +0x0C: 관직 / 품관 (0: 군주, 1~9: 1품관~9품관, Phase 5.9-B 실측 규명)
    uint8_t  loyalty;            // +0x0D: 충성도
    uint16_t birth_year;         // +0x0E: 출생년도
    uint16_t appear_year;        // +0x10: 등장년도
    uint16_t death_year;         // +0x12: 사망년도
    uint8_t  unknown_14;         // +0x14
    San13OfficerLiveStats stats; // +0x15 ~ +0x24: 16바이트 능력치 블록
    uint8_t  teach_skill;        // +0x25: 전수 특기 ID (0~19)
    uint8_t  skills[20];         // +0x26 ~ +0x39: 20대 특기 레벨 블록 (0~9, Phase 6.0 실측 규명)
    uint8_t  minister_trait_1;   // +0x3A: 고유 중신특성 ID 1
    uint8_t  minister_trait_2;   // +0x3B: 고유 중신특성 ID 2
    uint16_t force_id;           // +0x3C: 실제 소속 세력/군단 ID (EDGE-23 실측 특정 완료, 0=무소속)
    uint16_t lord_officer_id;    // +0x3E: 소속 군주 무장 ID (본인이 군주면 본인 ID와 일치, EDGE-31 실측 완료)
    uint16_t father_id;          // +0x40: 부친 무장 ID (0xFFFF = 없음)
    uint16_t mother_id;          // +0x42: 모친 무장 ID (0xFFFF = 없음)
    uint16_t spouse_1_id;        // +0x44: 배우자 1 무장 ID (0xFFFF = 없음)
    uint16_t spouse_2_id;        // +0x46: 배우자 2 무장 ID (0xFFFF = 없음)
    uint16_t spouse_3_id;        // +0x48: 배우자 3 무장 ID (0xFFFF = 없음)
    uint16_t sworn_brother_id;   // +0x4A: 의형제/인연 리더 무장 ID (0xFFFF = 없음)
    uint16_t rival_id;           // +0x4C: 원수 무장 ID (0xFFFF = 없음)
    uint8_t  personality;        // +0x4E: 성격 (0:소심, 1:냉정, 2:대담, 3:저돌, Phase 6.0 실측 규명)
    uint8_t  activity_flag;      // +0x4F: 활동 상태 (0x01: 활동/현역/재야, 0x00: 미등장, 0xFF: 미할당)
};

/**
 * @brief 무장 성향/기호/가치관 블록 (오프셋 +0x50 ~ +0x5F, 16바이트)
 */
struct San13OfficerCharacter {
    uint8_t  like_weapon;        // +0x00 (+0x50): 선호무기 (0:무관심, 1:관심)
    uint8_t  like_book;          // +0x01 (+0x51): 선호서적 (0:무관심, 1:관심)
    uint8_t  like_treasure;      // +0x02 (+0x52): 선호보물 (0:무관심, 1:관심)
    uint8_t  drink_level;        // +0x03 (+0x53): 음주/주량 (0:금주, 1:보통, 2:대음)
    uint8_t  greed;              // +0x04 (+0x54): 물욕/재물 (0:무욕, 1:보통, 2:강욕)
    uint8_t  loyalty_justice;    // +0x05 (+0x55): 의리 (0:경시, 1:낮음, 2:보통, 3:중시, 4:매우중시)
    uint8_t  valuing_courage;    // +0x06 (+0x56): 용애 (0:경시, 1:낮음, 2:보통, 3:중시, 4:매우중시)
    uint8_t  valuing_talent;     // +0x07 (+0x57): 재애 (0:경시, 1:낮음, 2:보통, 3:중시, 4:매우중시)
    uint8_t  affinity;           // +0x08 (+0x58): 상성 (0~149, 유비 3형제=50, 손견/동탁=100)
    uint8_t  pad_59_5f[7];       // +0x09 ~ +0x0F (+0x59 ~ +0x5F): 패딩
};

#pragma pack(pop)

/**
 * @brief 무장 메모리 배치 상수 정의
 */
struct OfficerMemoryLayout {
    static constexpr uintptr_t POOL_OFFSET = 0x015EE0;      // RootBase로부터의 무장 풀 시작 오프셋
    static constexpr size_t    OBJECT_STRIDE = 0x12BC;      // 무장 1명 오브젝트 크기 (4,796 바이트)
    static constexpr size_t    STATS_OFFSET = 0x15;         // 오브젝트 시작점으로부터 스탯 블록 오프셋
    static constexpr uint16_t  MAX_HISTORICAL_COUNT = 800;  // 역사 무장 최대 수
    static constexpr uint16_t  MAX_TOTAL_SLOTS = 2165;      // 신무장/자녀 무장 포함 전체 슬롯 상한
};

/**
 * @brief 에디터 UI 및 비즈니스 로직용 고수준 무장 레코드
 */
struct OfficerRecord {
    uint16_t id = 0;
    std::string name;
    uint16_t force_id = 0xFFFF;  // 실제 소속 세력/군단 ID (+0x3C, 0 또는 0xFFFF = 무소속/재야)
    uint16_t lord_officer_id = 0xFFFF; // 소속 군주 무장 ID (+0x3E, 본인이 군주면 본인 ID와 일치, EDGE-31)
    uint16_t face_id = 0;        // 얼굴 CG 번호 (+0x08, 조조 405, 관우 117 등)
    uint8_t  personality = 0;    // 무장 성격 (+0x4E: 0소심, 1냉정, 2대담, 3저돌)
    uint8_t  skills[20] = {0};   // 20대 특기 레벨 (+0x26 ~ +0x39, 0~9, Phase 6.0 실측 규명)
    uint8_t  teach_skill = 0;    // 전수 특기 (+0x25)
    uint8_t  minister_trait_1 = 0; // 중신특성 1 (+0x3A)
    uint8_t  minister_trait_2 = 0; // 중신특성 2 (+0x3B)
    uint8_t  tactic_id = 0;      // 보유 전법 ID (+0x24, 0~140, Step 6.0-2 규명)
    uint8_t  tactic_count = 0;   // 보유 전법 수 (+0x53, Step 6.0-2 규명 성과)
    uint8_t  tactic_type = 0;    // 전법/병종 계열 유형 (+0x56, Step 6.0-2 규명 성과)
    uint16_t father_id = 0xFFFF; // 부친 무장 ID (+0x40)
    uint16_t mother_id = 0xFFFF; // 모친 무장 ID (+0x42)
    uint16_t spouse_1_id = 0xFFFF; // 배우자 1 무장 ID (+0x44)
    uint16_t spouse_2_id = 0xFFFF; // 배우자 2 무장 ID (+0x46)
    uint16_t spouse_3_id = 0xFFFF; // 배우자 3 무장 ID (+0x48)
    uint16_t sworn_brother_id = 0xFFFF; // 의형제/인연 리더 무장 ID (+0x4A)
    uint16_t rival_id = 0xFFFF;  // 원수 무장 ID (+0x4C)
    San13OfficerCharacter character{}; // 8대 성향/기호/가치관/상성 (+0x50 ~ +0x58)
    uint16_t city_id = 0xFFFF;
    uint8_t  rank = 0;           // 관직 품관 (0: 군주, 1~9: 1품관~9품관)
    uint8_t  loyalty = 0;
    uint16_t birth_year = 0;
    uint16_t appear_year = 0;
    uint16_t death_year = 0;
    uint8_t  status_flag = 0;    // activity_flag (1: 활동/현역, 0: 미등장, 0xFF: 미할당)
    uint16_t current_year = 0;   // 현재 시나리오 연도 (EDGE-26/27 연동)
    bool     is_extended = false;
    San13OfficerLiveStats stats{};
    uintptr_t memory_address = 0;

    int GetTotalStats() const {
        return stats.curr_leadership + stats.curr_war + stats.curr_intel + stats.curr_politics;
    }

    const char* GetPersonalityDescription() const {
        static const char* const PERS[] = { "소심", "냉정", "대담", "저돌" };
        if (personality < 4) return PERS[personality];
        return "미상";
    }

    const char* GetDrinkDescription() const {
        static const char* const DRINK[] = { "금주", "보통", "대음" };
        if (character.drink_level < 3) return DRINK[character.drink_level];
        return "미상";
    }

    const char* GetGreedDescription() const {
        static const char* const GREED[] = { "무욕", "보통", "강욕" };
        if (character.greed < 3) return GREED[character.greed];
        return "미상";
    }

    const char* GetLoyaltyJusticeDescription() const {
        static const char* const VALS[] = { "경시", "낮음", "보통", "중시", "매우중시" };
        if (character.loyalty_justice < 5) return VALS[character.loyalty_justice];
        return "미상";
    }

    const char* GetValuingCourageDescription() const {
        static const char* const VALS[] = { "경시", "낮음", "보통", "중시", "매우중시" };
        if (character.valuing_courage < 5) return VALS[character.valuing_courage];
        return "미상";
    }

    const char* GetValuingTalentDescription() const {
        static const char* const VALS[] = { "경시", "낮음", "보통", "중시", "매우중시" };
        if (character.valuing_talent < 5) return VALS[character.valuing_talent];
        return "미상";
    }

    static const char* GetSkillName(size_t idx) {
        static const char* const SKILL_NAMES[20] = {
            "농업", "상업", "문화", "훈련", "순찰",
            "설파", "교섭", "언변", "덕망", "위풍",
            "신속", "분전", "연전", "공성", "병기",
            "견수", "수련", "일기", "호걸", "귀모"
        };
        if (idx < 20) return SKILL_NAMES[idx];
        return "특기";
    }

    static const char* GetTacticName(size_t idx) {
        static const char* const TACTIC_NAMES[141] = {
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
        if (idx < 141) return TACTIC_NAMES[idx];
        return "미상 전법";
    }

    const char* GetCurrentTacticDescription() const {
        return GetTacticName(tactic_id);
    }

    const char* GetStatusDescription(uint16_t overrideYear = 0) const {
        if (status_flag == 0xFF) return "미할당";

        uint16_t yr = overrideYear > 0 ? overrideYear : current_year;
        if (yr > 0) {
            // [EDGE-26] 등장년도가 현재 연도보다 미래이면 무조건 미등장 (아회남 해결!)
            if (appear_year > yr) return "미등장";
            // 사망년도 경과 시 사망
            if (death_year > 0 && death_year <= yr) return "사망";
        }

        // [EDGE-29 & EDGE-31] 세력 미사관(status_flag==0), 무소속(force_id==0/0xFFFF), 또는 미거병 가상세력(force_id >= 120)은 재야
        // 범증(120), 우희(125), 항적(125), 손책(125) 등 인게임 재야 완벽 해결!
        if (status_flag == 0 || force_id == 0 || force_id == 0xFFFF || force_id >= 120) {
            return "재야";
        }

        // status_flag == 1 이고 유효 세력 소속(force_id < 120)이 있는 실제 현역 사관 무장
        if (rank == 0) {
            // [EDGE-31] 군주 본인 ID와 일치할 때만 "군주", 원희/원상 등 군주 친족 무품관은 "일반"
            if (lord_officer_id == id || lord_officer_id == 0xFFFF) {
                return "군주";
            }
            return "일반";
        }

        // Phase 5.9-B: 삼국지 13 PK 공식 1품관~9품관 통합 표기 매핑
        static const char* const RANK_DESCS[] = {
            "군주",
            "일반 (1품관)",
            "일반 (2품관)",
            "일반 (3품관)",
            "일반 (4품관)",
            "일반 (5품관)",
            "일반 (6품관)",
            "일반 (7품관)",
            "일반 (8품관)",
            "일반 (9품관)"
        };
        if (rank < 10) {
            return RANK_DESCS[rank];
        }
        return "일반";
    }

    // 적성 문자열 헬퍼
    static const char* AptitudeToString(uint8_t apt) {
        static const char* APT_NAMES[] = { "C", "B", "A", "S" };
        return APT_NAMES[apt & 0x03];
    }
};

/**
 * @brief 일괄 수정 전 원상 복구(Undo) 및 트랜잭션 롤백을 위한 무장 상태 스냅샷
 */
struct OfficerSnapshot {
    uint16_t              id = 0;
    San13OfficerLiveStats stats{};
    uint16_t              birth_year = 0;
    uint16_t              appear_year = 0;
    uint16_t              death_year = 0;
};

/**
 * @brief 유명 무장 이름 매핑 헬퍼
 */
const char* GetFamousOfficerName(uint16_t id);

} // namespace San13::Game
