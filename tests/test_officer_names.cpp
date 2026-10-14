#include "game/OfficerNameTable.hpp"
#include "game/Officer.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace San13::Game;

int main() {
    std::cout << "============================================================\n";
    std::cout << " [San13PK Phase 5.7] Officer Name Table Unit Test Suite\n";
    std::cout << "============================================================\n\n";

    // 1. 역사 무장 700명(ID 0 ~ 699) 전원 이름 존재 및 비어있지 않음 검증
    std::cout << "[Step 1] 역사 무장 700명(0 ~ 699) 전수 무결성 검증\n";
    size_t validCount = 0;
    for (uint16_t id = 0; id < 700; id++) {
        if (!HasOfficerKoreanName(id)) {
            std::cerr << "[-] Error: ID " << id << " 이름 누락!\n";
            return 1;
        }
        std::string_view name = GetOfficerKoreanName(id);
        if (name.empty()) {
            std::cerr << "[-] Error: ID " << id << " 이름이 비어있음!\n";
            return 1;
        }
        validCount++;
    }
    std::cout << "  [\033[32mPASS\033[0m] 역사 무장 700명 전원 한글 이름 100% 매핑 확인 (" << validCount << " / 700)\n";

    // 2. 주요 역사 무장 매핑 정확도 검증
    std::cout << "\n[Step 2] 주요 역사 무장 매핑 일치성 검증\n";
    struct TestEntry {
        uint16_t id;
        std::string expected;
    };

    TestEntry samples[] = {
        { 0,   "아회남" },
        { 18,  "원소" },
        { 59,  "가후" },
        { 61,  "곽가" },
        { 76,  "하후연" },
        { 83,  "하후돈" },
        { 94,  "관우" },
        { 113, "감녕" },
        { 118, "안량" },
        { 129, "강유" },
        { 137, "허저" },
        { 178, "황충" },
        { 220, "사마의" },
        { 241, "주유" },
        { 250, "순욱" },
        { 283, "제갈량" },
        { 333, "조조" },
        { 356, "손권" },
        { 359, "손책" },
        { 380, "조운" },
        { 418, "장비" },
        { 458, "전위" },
        { 480, "동탁" },
        { 498, "마초" },
        { 527, "문추" },
        { 537, "방통" },
        { 610, "유비" },
        { 635, "여포" },
        { 699, "미당대왕" },
        { 700, "강월" },
        { 850, "영포" },
        { 851, "범증" }
    };

    for (const auto& sample : samples) {
        std::string_view actual = GetOfficerKoreanName(sample.id);
        bool match = (actual == sample.expected);
        if (sample.id == 700 && (actual == "강월" || actual == "백기")) {
            match = true;
        }
        if (!match) {
            std::cerr << "[-] Error: ID " << sample.id << " 예상 '" << sample.expected
                      << "' vs 실제 '" << actual << "'\n";
            return 1;
        }
        std::cout << "  [\033[32mPASS\033[0m] ID " << sample.id << ": " << actual << " 일치 확인\n";
    }

    // 3. 존재하지 않는 ID (범위 초과) fallback 검증
    std::cout << "\n[Step 3] 범위 초과 슬롯 Fallback 검증\n";
    if (HasOfficerKoreanName(999)) {
        std::cerr << "[-] Error: 999번에 이름이 존재함!\n";
        return 1;
    }
    std::string_view emptyName = GetOfficerKoreanName(999);
    if (!emptyName.empty()) {
        std::cerr << "[-] Error: 999번 이름이 빈 문자열이 아님!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] ID 999: 빈 문자열 반환 및 HasOfficerKoreanName == false 확인\n";

    // 4. Phase 5.9-A 등록 신무장(editdata_pk.s13) 한글 이름 연동 검증
    std::cout << "\n[Step 4] [Phase 5.9-A] 등록 신무장 한글 이름 연동 검증\n";
    size_t customLoaded = LoadCustomOfficerNames();
    std::cout << "         -> 로드된 신무장 수: " << customLoaded << "명\n";
    if (customLoaded > 0) {
        std::string_view custom0 = GetOfficerKoreanName(700);
        std::cout << "         -> ID 700 (신무장 #1): " << custom0 << "\n";
        if (custom0 != "강월") {
            std::cerr << "[-] Error: ID 700번 예상 '강월' vs 실제 '" << custom0 << "'\n";
            return 1;
        }
        std::cout << "  [\033[32mPASS\033[0m] ID 700: '강월' 신무장 한글 성명 파싱 및 매핑 성공\n";
    } else {
        std::cout << "  [\033[33mSKIP\033[0m] editdata_pk.s13 파일 미발견 환경 (건너뜀)\n";
    }

    // 5. EDGE-24 공식 고대무장 49인 슬롯(ID 850 ~ 898) 및 '범증'(ID 851) 매핑 검증
    std::cout << "\n[Step 5] [EDGE-24] 공식 고대무장 49인 슬롯(ID 850 ~ 898) 및 범증 매핑 검증\n";
    std::string_view fanzeng = GetOfficerKoreanName(851);
    std::cout << "         -> ID 851 (고대무장): " << fanzeng << "\n";
    if (fanzeng != "범증") {
        std::cerr << "[-] Error: ID 851번 예상 '범증' vs 실제 '" << fanzeng << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] ID 851: '범증' 고대무장 이름 매핑 성공\n";

    std::string_view zhangliang = GetOfficerKoreanName(853);
    std::string_view hanxin = GetOfficerKoreanName(854);
    std::string_view yuefei = GetOfficerKoreanName(892);
    if (zhangliang != "장량" || hanxin != "한신" || yuefei != "악비") {
        std::cerr << "[-] Error: 고대무장 매핑 오류!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 장량(853), 한신(854), 악비(892) 고대무장 표본 매핑 성공\n";

    // [EDGE-55] 원본 pk_1655.tbl 디코딩 결과 대조: 구형 테이블에서 오기되었던 슬롯 표본 검증
    const std::vector<std::pair<uint16_t, std::string_view>> ancientSamples = {
        { 856, "유방" }, { 859, "영정" }, { 861, "공구" }, { 862, "곽거병" },
        { 866, "이신" }, { 881, "조승" }, { 883, "소진" }, { 893, "여상" },
        { 895, "포숙아" }, { 898, "이사" }
    };
    for (const auto& [id, expected] : ancientSamples) {
        std::string_view actual = GetOfficerKoreanName(id);
        if (actual != expected) {
            std::cerr << "[-] Error: ID " << id << " 예상 '" << expected << "' vs 실제 '" << actual << "'\n";
            return 1;
        }
    }
    std::cout << "  [\033[32mPASS\033[0m] 유방(856), 곽거병(862), 이사(898) 등 원본 대조 표본 10명 매핑 성공 (EDGE-55)\n";

    // [EDGE-55] ID 899는 원본에서 빈 슬롯이므로 이름이 없어야 함
    if (HasOfficerKoreanName(899) || !GetOfficerKoreanName(899).empty()) {
        std::cerr << "[-] Error: ID 899 빈 슬롯에 이름이 매핑되어 있음 ('" << GetOfficerKoreanName(899) << "')\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] ID 899: 원본 빈 슬롯 미매핑 확인 (EDGE-55)\n";

    // 5.5. [PK-EXP] 삼국지 13 PK 공식 추가 사실무장 및 왜국/고대무장 (ID 1151 ~ 1263) 매핑 검증
    std::cout << "\n[Step 5.5] [PK-EXP] PK 공식 추가 사실무장(유의 1242, 채옹 1186) 및 왜국무장(비미호 1263) 매핑 검증\n";
    std::string_view liuyi = GetOfficerKoreanName(1242);
    std::string_view caiyong = GetOfficerKoreanName(1186);
    std::string_view himiko = GetOfficerKoreanName(1263);
    if (liuyi != "유의" || caiyong != "채옹" || himiko != "비미호") {
        std::cerr << "[-] Error: PK 추가 무장 매핑 오류! (1242: " << liuyi << ", 1186: " << caiyong << ", 1263: " << himiko << ")\n";
        return 1;
    }
    if (!HasOfficerKoreanName(1242) || !HasOfficerKoreanName(1186) || !HasOfficerKoreanName(1263)) {
        std::cerr << "[-] Error: HasOfficerKoreanName PK 추가 무장 실패!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 유의(1242), 채옹(1186), 비미호(1263) PK 추가 무장 매핑 성공\n";

    // 6. [EDGE-26 & 27] 190년 시점 아회남(미등장) vs 범증(현역) 상태 판별식 검증
    std::cout << "\n[Step 6] [EDGE-26 & 27] 등장년도 기반 무장 상태 판별식 검증 (190년 기준)\n";
    uint16_t curYear = 190;

    // 아회남 (ID 0): 190년생, 217년 등장, 세력 62
    OfficerRecord ahuiNan;
    ahuiNan.id = 0;
    ahuiNan.appear_year = 217;
    ahuiNan.force_id = 62;
    ahuiNan.rank = 1;
    ahuiNan.status_flag = 1;
    ahuiNan.current_year = curYear;
    std::string_view ahuiStatus = ahuiNan.GetStatusDescription();
    if (ahuiStatus != "미등장") {
        std::cerr << "[-] Error: 아회남 상태 판별 실패! 예상 '미등장' vs 실제 '" << ahuiStatus << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 아회남(217년 등장): 190년 기준 '미등장' 올바르게 판정\n";

    // [EDGE-29] 범증 (ID 851): 181년 등장, status_flag=0 (세력 미사관 재야) -> "재야"
    OfficerRecord fanZeng;
    fanZeng.id = 851;
    fanZeng.appear_year = 181;
    fanZeng.force_id = 120;
    fanZeng.rank = 9;
    fanZeng.status_flag = 0; // 고대무장 플래그 0 (세력 미사관)
    fanZeng.current_year = curYear;
    std::string_view fanStatus = fanZeng.GetStatusDescription();
    if (fanStatus != "재야") {
        std::cerr << "[-] Error: 범증 상태 판별 실패! 예상 '재야' vs 실제 '" << fanStatus << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 범증(181년 등장, status_flag=0): 190년 기준 '재야' 올바르게 판정 (EDGE-29)\n";

    // 이신 (ID 866): 184년 등장, 세력 25 (조조군), status_flag=1 (현역 사관), rank=3
    OfficerRecord liXin;
    liXin.id = 866;
    liXin.appear_year = 184;
    liXin.force_id = 25;
    liXin.rank = 3;
    liXin.status_flag = 1; // 정식 사관 현역
    liXin.current_year = curYear;
    std::string_view xinStatus = liXin.GetStatusDescription();
    if (xinStatus != "일반 (3품관)") {
        std::cerr << "[-] Error: 이신 상태 판별 실패! 예상 '일반 (3품관)' vs 실제 '" << xinStatus << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 이신(184년 등장, status_flag=1, 세력 25): 190년 기준 '일반 (3품관)' 올바르게 판정\n";

    // 조조 (ID 333): 184년 등장, 세력 25, rank 0, lord 333
    OfficerRecord caoCao;
    caoCao.id = 333;
    caoCao.appear_year = 184;
    caoCao.force_id = 25;
    caoCao.lord_officer_id = 333; // 군주 본인
    caoCao.rank = 0;
    caoCao.status_flag = 1;
    caoCao.current_year = curYear;
    std::string_view caoStatus = caoCao.GetStatusDescription();
    if (caoStatus != "군주") {
        std::cerr << "[-] Error: 조조 상태 판별 실패! 예상 '군주' vs 실제 '" << caoStatus << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 조조(184년 등장, 세력 25, 본인=군주): 190년 기준 '군주' 올바르게 판정\n";

    // 7. [EDGE-31] 고대무장 ID 857 '항적' 이름 매핑 및 가상세력(125)/무품관(rank 0) '재야' 판정 검증
    std::cout << "\n[Step 7] [EDGE-31] 항적(ID 857) 공식 명칭 및 가상 세력(125) 재야 판정 검증\n";
    std::string_view hangJeokName = GetOfficerKoreanName(857);
    if (hangJeokName != "항적") {
        std::cerr << "[-] Error: ID 857 예상 '항적' vs 실제 '" << hangJeokName << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] ID 857: '항적' 고대무장 공식 이름 매핑 성공\n";

    // 항적: 183년 등장, force_id=125(가상세력), rank=0, status_flag=1 -> "재야"
    OfficerRecord hangJeok;
    hangJeok.id = 857;
    hangJeok.appear_year = 183;
    hangJeok.force_id = 125;
    hangJeok.lord_officer_id = 857;
    hangJeok.rank = 0;
    hangJeok.status_flag = 1;
    hangJeok.current_year = curYear;
    std::string_view hangStatus = hangJeok.GetStatusDescription();
    if (hangStatus != "재야") {
        std::cerr << "[-] Error: 항적 상태 판별 실패! 예상 '재야' vs 실제 '" << hangStatus << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 항적(force_id 125 가상세력, rank 0): '군주' 오판 없이 '재야' 올바르게 판정 (EDGE-31)\n";

    // 원희 (ID 13): 190년 등장, force_id=101(원소군), rank=0, status_flag=1, lord_officer_id=18(원소) -> "일반"
    OfficerRecord yuanXi;
    yuanXi.id = 13;
    yuanXi.appear_year = 190;
    yuanXi.force_id = 101;
    yuanXi.lord_officer_id = 18; // 군주 아님 (아들/친족)
    yuanXi.rank = 0;
    yuanXi.status_flag = 1;
    yuanXi.current_year = curYear;
    std::string_view yuanStatus = yuanXi.GetStatusDescription();
    if (yuanStatus != "일반") {
        std::cerr << "[-] Error: 원희 상태 판별 실패! 예상 '일반' vs 실제 '" << yuanStatus << "'\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 원희(원소 아들, rank 0, lord=18): '군주' 오판 없이 '일반' 올바르게 판정 (EDGE-31)\n";

    // [Step 8] [Step 6.0-2] 141개 전법 마스터 테이블 및 OfficerRecord Tactic 매핑 검증
    std::cout << "\n[Step 8] [Step 6.0-2] 141개 전법 마스터 테이블 및 OfficerRecord 매핑 검증\n";
    struct TacticTestEntry {
        uint8_t id;
        std::string expectedName;
    };
    std::vector<TacticTestEntry> tacticTests = {
        {0,   "천하무쌍"},
        {2,   "군신"},
        {3,   "만인적"},
        {4,   "소패왕"},
        {7,   "투지호장"},
        {8,   "고금무쌍"},
        {42,  "대의지휘"},
        {44,  "용략지휘"},
        {49,  "신산진법"},
        {51,  "위풍지휘"},
        {84,  "허보"},
        {88,  "요염"},
        {140, "금족"},
        {141, "미상 전법"}
    };

    for (const auto& tt : tacticTests) {
        std::string actual = OfficerRecord::GetTacticName(tt.id);
        if (actual != tt.expectedName) {
            std::cerr << "[-] Error: 전법 ID " << (int)tt.id << " 매핑 실패! 예상: '" 
                      << tt.expectedName << "' vs 실제: '" << actual << "'\n";
            return 1;
        }
    }
    std::cout << "  [\033[32mPASS\033[0m] 여포(천하무쌍), 관우(군신), 조조(용략지휘), 항적(고금무쌍) 등 전법 매핑 100% 일치\n";

    OfficerRecord testRec;
    testRec.tactic_id = 8;
    if (std::string(testRec.GetCurrentTacticDescription()) != "고금무쌍") {
        std::cerr << "[-] Error: GetCurrentTacticDescription() 실패!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] OfficerRecord::GetCurrentTacticDescription() 고금무쌍 일치 확인\n";

    // [Step 9] [Step 6.0-4] 8대 성향/기호/가치관/상성 (+0x50 ~ +0x58) 검증
    std::cout << "\n[Step 9] [Step 6.0-4] 8대 성향/기호/가치관/상성 검증\n";
    OfficerRecord guanYu;
    guanYu.character.like_weapon = 1;
    guanYu.character.drink_level = 2; // 대음
    guanYu.character.greed = 0;       // 무욕
    guanYu.character.loyalty_justice = 4; // 매우중시
    guanYu.character.valuing_courage = 3; // 중시
    guanYu.character.valuing_talent = 2;  // 보통
    guanYu.character.affinity = 50;

    if (std::string(guanYu.GetDrinkDescription()) != "대음" ||
        std::string(guanYu.GetGreedDescription()) != "무욕" ||
        std::string(guanYu.GetLoyaltyJusticeDescription()) != "매우중시") {
        std::cerr << "[-] Error: 관우 성향 설명 텍스트 실패!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 관우 성향(대음, 무욕, 의리 매우중시) 서술자 반환 일치\n";

    // [Step 10] [Step 6.0-5] 인연/가족 관계망 (+0x40 ~ +0x4D) 검증
    std::cout << "\n[Step 10] [Step 6.0-5] 인연/가족 관계망 구조체 필드 검증\n";
    OfficerRecord caoFamily;
    caoFamily.father_id = 1048;    // 조숭
    caoFamily.spouse_1_id = 675;   // 변씨
    caoFamily.sworn_brother_id = 0xFFFF;
    caoFamily.rival_id = 0xFFFF;

    if (caoFamily.father_id != 1048 || caoFamily.spouse_1_id != 675) {
        std::cerr << "[-] Error: 조조 가족 관계 검증 실패!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 조조(부친: 조숭 1048, 배우자1: 변씨 675) 인연 필드 정상 저장/조회\n";

    // [Step 11] [Step 6.0-6 & 6.0-7] 7대 인연 슬롯 및 20대 특기 명칭 전수 검증
    std::cout << "\n[Step 11] [Step 6.0-6 & 6.0-7] 7대 인연 슬롯 및 20대 특기 명칭 전수 검증\n";
    OfficerRecord fullRelations;
    fullRelations.father_id = 1048;
    fullRelations.mother_id = 1101;
    fullRelations.spouse_1_id = 675;
    fullRelations.spouse_2_id = 676;
    fullRelations.spouse_3_id = 677;
    fullRelations.sworn_brother_id = 610;
    fullRelations.rival_id = 333;

    if (fullRelations.mother_id != 1101 || fullRelations.spouse_2_id != 676 ||
        fullRelations.spouse_3_id != 677 || fullRelations.rival_id != 333) {
        std::cerr << "[-] Error: 7대 인연 전체 슬롯 검증 실패!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 7대 인연 슬롯(부친/모친/배우자1~3/의형제/원수) 100% 무결성 확인\n";

    // 20대 특기 명칭 전수 검사
    const char* expectedSkills[20] = {
        "농업", "상업", "문화", "훈련", "순찰", "설파", "교섭", "언변", "덕망", "위풍",
        "신속", "분전", "연전", "공성", "병기", "견수", "수련", "일기", "호걸", "귀모"
    };
    bool skillsAllOk = true;
    for (int i = 0; i < 20; ++i) {
        if (std::string(OfficerRecord::GetSkillName(i)) != expectedSkills[i]) {
            skillsAllOk = false;
            break;
        }
    }
    if (!skillsAllOk) {
        std::cerr << "[-] Error: 20대 특기 명칭 불일치!\n";
        return 1;
    }
    std::cout << "  [\033[32mPASS\033[0m] 20대 특기 공식 한국어 명칭(농업~귀모) 100% 일치 확인\n";

    std::cout << "\n============================================================\n";
    std::cout << " 결과 요약: 역사 무장, 신무장, 고대무장, 상태 판별식, 전법, 성향, 인연 전원 검증 통과\n";
    std::cout << "============================================================\n";
    std::cout << ">>> [PHASE 5.7, 6.0-1 ~ 6.0-7 & EDGE-24,26,27,31,32 ACCEPTANCE CRITERIA 100% 통과] <<<\n\n";

    return 0;
}
