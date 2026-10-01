#include "ui/HangulAutomata.hpp"
#include <iostream>
#include <cassert>
#include <string>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  \033[31m[FAIL]\033[0m " << msg << " (Line " << __LINE__ << ")\n"; \
            return false; \
        } else { \
            std::cout << "  \033[32m[PASS]\033[0m " << msg << "\n"; \
        } \
    } while(0)

using namespace San13::UI;

static bool TestFamousOfficerNames() {
    std::cout << "\n=== [Test Set 1] 삼국지 주요 무장 성명 두벌식 오토마타 변환 검증 ===\n";

    // 조조 (whwh)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("whwh");
        TEST_ASSERT(res == "조조", "조조 (whwh) -> '조조' 일치 (결과: " + res + ")");
    }

    // 유비 (dbql)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("dbql");
        TEST_ASSERT(res == "유비", "유비 (dbql) -> '유비' 일치 (결과: " + res + ")");
    }

    // 관우 (rhksdn) - 복모음 ㅘ(ㅗ+ㅏ) 포함
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("rhksdn");
        TEST_ASSERT(res == "관우", "관우 (rhksdn) -> '관우' 일치 (결과: " + res + ")");
    }

    // 손권 (thsrnjs) - 복모음 ㅝ(ㅜ+ㅓ) 포함
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("thsrnjs");
        TEST_ASSERT(res == "손권", "손권 (thsrnjs) -> '손권' 일치 (결과: " + res + ")");
    }

    // 제갈량 (wprkffid)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("wprkffid");
        TEST_ASSERT(res == "제갈량", "제갈량 (wprkffid) -> '제갈량' 일치 (결과: " + res + ")");
    }

    // 하후돈 (gkgnehs)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("gkgnehs");
        TEST_ASSERT(res == "하후돈", "하후돈 (gkgnehs) -> '하후돈' 일치 (결과: " + res + ")");
    }

    return true;
}

static bool TestCityNames() {
    std::cout << "\n=== [Test Set 2] 삼국지 주요 도시 성명 변환 검증 ===\n";

    // 낙양 (skrdid)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("skrdid");
        TEST_ASSERT(res == "낙양", "낙양 (skrdid) -> '낙양' 일치 (결과: " + res + ")");
    }

    // 장안 (wkddks)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("wkddks");
        TEST_ASSERT(res == "장안", "장안 (wkddks) -> '장안' 일치 (결과: " + res + ")");
    }

    // 건업 (rjsdjq)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("rjsdjq");
        TEST_ASSERT(res == "건업", "건업 (rjsdjq) -> '건업' 일치 (결과: " + res + ")");
    }

    // 한중 (gkswnd)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("gkswnd");
        TEST_ASSERT(res == "한중", "한중 (gkswnd) -> '한중' 일치 (결과: " + res + ")");
    }

    return true;
}

static bool TestCompoundJamoAndEdgeCases() {
    std::cout << "\n=== [Test Set 3] 복합 자음(종성) 및 복모음 조합 검증 ===\n";

    // 닭 (ekfr) -> ㄺ (ㄹ + ㄱ)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("ekfr");
        TEST_ASSERT(res == "닭", "닭 (ekfr) -> '닭' (복합받침 ㄺ) 일치 (결과: " + res + ")");
    }

    // 값 (rkqt) -> ㅄ (ㅂ + ㅅ)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("rkqt");
        TEST_ASSERT(res == "값", "값 (rkqt) -> '값' (복합받침 ㅄ) 일치 (결과: " + res + ")");
    }

    // 몫 (ahrt) -> ㄳ (ㄱ + ㅅ)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("ahrt");
        TEST_ASSERT(res == "몫", "몫 (ahrt) -> '몫' (복합받침 ㄳ) 일치 (결과: " + res + ")");
    }

    // 앉다 (dkswek) -> ㄵ (ㄴ + ㅈ)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("dkswek");
        TEST_ASSERT(res == "앉다", "앉다 (dkswek) -> '앉다' 일치 (결과: " + res + ")");
    }

    // 의사 (dmltk) -> 복모음 ㅢ (ㅡ + ㅣ)
    {
        std::string res = HangulAutomata::ConvertQwertyToHangul("dmltk");
        TEST_ASSERT(res == "의사", "의사 (dmltk) -> '의사' 일치 (결과: " + res + ")");
    }

    return true;
}

static bool TestConsonantTransition() {
    std::cout << "\n=== [Test Set 4] [EDGE-17] 모음 입력시 연음 분리(Consonant Transition) 검증 ===\n";

    // 1. 단일 받침 연음: 각(rkr) + 아(k) -> 가가
    {
        HangulAutomata a;
        a.FeedString("rkr"); // 각
        TEST_ASSERT(a.GetText() == "각", "초기 입력: '각'");
        a.ProcessKey('k');   // ㅏ 입력 -> '가' + '가'
        TEST_ASSERT(a.GetText() == "가가", "단일 받침 연음: '각' + 'ㅏ' -> '가가' 일치 (결과: " + a.GetText() + ")");
    }

    // 2. 복합 받침 연음: 닭(ekfr) + 이(l) -> 달기
    {
        HangulAutomata a;
        a.FeedString("ekfr"); // 닭
        TEST_ASSERT(a.GetText() == "닭", "초기 입력: '닭'");
        a.ProcessKey('l');    // ㅣ 입력 -> '달' + '기'
        TEST_ASSERT(a.GetText() == "달기", "복합 받침 연음: '닭' + 'ㅣ' -> '달기' 일치 (결과: " + a.GetText() + ")");
    }

    // 3. 복합 받침 연음: 값(rkqt) + 이(l) -> 갑시
    {
        HangulAutomata a;
        a.FeedString("rkqt"); // 값
        TEST_ASSERT(a.GetText() == "값", "초기 입력: '값'");
        a.ProcessKey('l');    // ㅣ 입력 -> '갑' + '시'
        TEST_ASSERT(a.GetText() == "갑시", "복합 받침 연음: '값' + 'ㅣ' -> '갑시' 일치 (결과: " + a.GetText() + ")");
    }

    return true;
}

static bool TestBackspaceDecomposition() {
    std::cout << "\n=== [Test Set 5] [EDGE-17] 백스페이스 1자모 역분해(Decomposition) 검증 ===\n";

    // '닭' (ekfr) 단계별 역분해: 닭 -> 달 -> 다 -> ㄷ -> 빈문자열
    {
        HangulAutomata a;
        a.FeedString("ekfr");
        TEST_ASSERT(a.GetText() == "닭", "초기 입력: '닭'");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText() == "달", "1차 백스페이스: '닭' -> '달' (복합받침 분해)");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText() == "다", "2차 백스페이스: '달' -> '다' (단일받침 제거)");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText() == "ㄷ", "3차 백스페이스: '다' -> 'ㄷ' (중성 제거)");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText().empty(), "4차 백스페이스: 'ㄷ' -> 빈문자열 (초성 제거)");
    }

    // '과' (rhk) 복모음 단계별 역분해: 과 -> 고 -> ㄱ -> 빈문자열
    {
        HangulAutomata a;
        a.FeedString("rhk");
        TEST_ASSERT(a.GetText() == "과", "초기 입력: '과'");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText() == "고", "1차 백스페이스: '과' -> '고' (복모음 ㅘ 분해)");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText() == "ㄱ", "2차 백스페이스: '고' -> 'ㄱ' (단모음 ㅗ 제거)");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText().empty(), "3차 백스페이스: 'ㄱ' -> 빈문자열 (초성 제거)");
    }

    // 확정 문자열 백스페이스: "조조" -> 백스페이스 -> "조" -> 백스페이스 -> ""
    {
        HangulAutomata a;
        a.SetText("조조");
        TEST_ASSERT(a.GetText() == "조조", "초기 확정 문자열: '조조'");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText() == "조", "1차 백스페이스: '조조' -> '조' (마지막 3바이트 UTF-8 문자 제거)");

        a.ProcessBackspace();
        TEST_ASSERT(a.GetText().empty(), "2차 백스페이스: '조' -> 빈문자열");
    }

    return true;
}

static bool TestHanEngToggleAndMixedTyping() {
    std::cout << "\n=== [Test Set 6] [EDGE-18] 한/영 모드 토글 및 영문/숫자 혼용 검증 ===\n";

    HangulAutomata a;
    TEST_ASSERT(a.IsHangulMode() == true, "기본 모드는 한글(Hangul) 모드 활성화");

    // 한글 입력: "조조"
    a.FeedString("whwh");
    TEST_ASSERT(a.GetText() == "조조", "한글 모드 입력: '조조'");

    // 영문 모드로 전환 (Right Alt / Shift+Space 모방)
    a.ToggleHangulMode();
    TEST_ASSERT(a.IsHangulMode() == false, "영문(English) 모드로 전환됨");
    TEST_ASSERT(a.GetText() == "조조", "모드 전환 시 기존 텍스트 온전히 커밋 보존");

    // 공백 및 영문 입력
    a.ProcessKey(' ');
    a.ProcessKey('C');
    a.ProcessKey('a');
    a.ProcessKey('o');
    a.ProcessKey(' ');
    a.ProcessKey('1');
    a.ProcessKey('2');
    a.ProcessKey('3');
    TEST_ASSERT(a.GetText() == "조조 Cao 123", "영문 모드 타이핑 보존: '조조 Cao 123'");

    // 다시 한글 모드로 전환
    a.ToggleHangulMode();
    TEST_ASSERT(a.IsHangulMode() == true, "다시 한글 모드로 복귀");
    a.ProcessKey(' ');
    a.FeedString("dbql"); // 유비
    TEST_ASSERT(a.GetText() == "조조 Cao 123 유비", "혼용 최종 결과: '조조 Cao 123 유비'");

    return true;
}

int main() {
    std::cout << "====================================================================\n";
    std::cout << "   Romance of the Three Kingdoms 13 PK - Hangul Automata Test Suite \n";
    std::cout << "====================================================================\n";

    int passed = 0;
    int total = 6;

    if (TestFamousOfficerNames()) passed++;
    if (TestCityNames()) passed++;
    if (TestCompoundJamoAndEdgeCases()) passed++;
    if (TestConsonantTransition()) passed++;
    if (TestBackspaceDecomposition()) passed++;
    if (TestHanEngToggleAndMixedTyping()) passed++;

    std::cout << "\n--------------------------------------------------------------------\n";
    std::cout << " 결과 요약: " << passed << " / " << total << " 테스트 세트 성공 (" 
              << (passed * 100 / total) << "%)\n";
    std::cout << "====================================================================\n";

    return (passed == total) ? 0 : 1;
}
