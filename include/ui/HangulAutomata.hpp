#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace San13::UI {

enum class AutomataState {
    Empty,               // 조합 중인 글자 없음
    ChoOnly,             // 초성만 입력됨 (예: 'ㄱ')
    ChoJung,             // 초성 + 중성 (단모음/복모음, 예: '가', '과')
    ChoJungJong,         // 초성 + 중성 + 단일 종성 (예: '간', '달')
    ChoJungDoubleJong    // 초성 + 중성 + 복합 종성 (예: '닭', '값')
};

/**
 * @brief 리눅스/Proton 환경 fcitx5/GLFW 한계 극복을 위한 네이티브 한글 2벌식 소프트웨어 오토마타
 * 
 * QWERTY 영문 입력을 표준 두벌식 자모로 실시간 변환하고,
 * 초성/중성/종성 결합, 복모음/복자음 조합, 연음 분리, 백스페이스 1자모 역분해를 완벽히 처리합니다.
 */
class HangulAutomata {
public:
    HangulAutomata();
    ~HangulAutomata() = default;

    /**
     * @brief 단일 QWERTY 키 입력 처리
     * @param c ASCII 영문 또는 특수문자
     * @return 키가 소비되어 상태가 변경되었으면 true
     */
    bool ProcessKey(char c);

    /**
     * @brief 백스페이스 1회 처리 (복합받침 -> 단일받침 -> 초성+중성 -> 초성 -> 삭제)
     * @return 텍스트 변화가 있었으면 true
     */
    bool ProcessBackspace();

    /**
     * @brief 영문 QWERTY 문자열을 순차적으로 입력 처리 (예: "whwh" -> "조조")
     */
    void FeedString(const std::string& qwerty);

    /**
     * @brief 현재 조합 중인 미완성 음절을 확정(Commit)하여 버퍼에 영구 반영
     */
    void Commit();

    /**
     * @brief 버퍼 및 조합 상태 완전 초기화
     */
    void Clear();

    /**
     * @brief 기존 텍스트 버퍼 설정 (편집 모달 또는 외부 입력 로드시 사용)
     */
    void SetText(const std::string& text);

    /**
     * @brief 전체 텍스트 반환 (이미 확정된 텍스트 + 현재 조합 중인 음절)
     */
    std::string GetText() const;

    /**
     * @brief 이미 확정 완료된 텍스트 반환
     */
    const std::string& GetCommittedText() const { return m_committedText; }

    /**
     * @brief 현재 실시간 조합 중인 음절 UTF-8 반환 (없을 시 빈 문자열)
     */
    std::string GetComposingSyllable() const;

    /**
     * @brief 음절 조합 중 여부
     */
    bool IsComposing() const { return m_state != AutomataState::Empty; }

    /**
     * @brief 현재 오토마타 상태 조회
     */
    AutomataState GetState() const { return m_state; }

    /**
     * @brief 한/영 모드 조회 및 설정
     */
    bool IsHangulMode() const { return m_hangulMode; }
    void SetHangulMode(bool hangul);
    void ToggleHangulMode() { SetHangulMode(!m_hangulMode); }

    /**
     * @brief 정적 순수 함수: QWERTY 문자열을 두벌식 한글로 즉각 변환
     */
    static std::string ConvertQwertyToHangul(const std::string& qwerty);

    /**
     * @brief 유니코드 코드포인트를 UTF-8 바이트 시퀀스로 변환
     */
    static std::string CodepointToUtf8(uint32_t cp);

    /**
     * @brief UTF-8 문자열의 마지막 글자 1자(1~4바이트)를 안전하게 삭제
     */
    static void PopLastUtf8Char(std::string& s);

private:
    void ResetComposing();

    bool m_hangulMode = true;
    std::string m_committedText;

    AutomataState m_state = AutomataState::Empty;
    int m_cho = -1;       // 초성 인덱스 (0..18)
    int m_jung = -1;      // 중성 인덱스 (0..20)
    int m_jong = 0;       // 종성 인덱스 (0..27, 0은 종성 없음)

    // 복합 자모 세부 분해 추적용
    int m_jung1 = -1;     // 복모음 제1요소
    int m_jung2 = -1;     // 복모음 제2요소 (-1이면 단모음)
    int m_jong1 = 0;      // 복합받침 제1요소
    int m_jong2 = 0;      // 복합받침 제2요소 (0이면 단일받침)
};

/**
 * @brief Dear ImGui 네이티브 2벌식 한글 입력 래퍼 위젯
 * 
 * QWERTY 영문 입력을 가로채어 실시간 한글 음절 조합, 1자모 백스페이스 역분해,
 * Right Alt / Shift+Space 한/영 전환 및 한/영 모드 시각화 뱃지를 지원합니다.
 * 
 * @param label ImGui 위젯 고유 식별자
 * @param hint 텍스트 필드 힌트/플레이스홀더
 * @param buf 대상 C 문자열 버퍼
 * @param buf_size 버퍼 크기
 * @param automata 해당 입력창의 상태를 유지할 HangulAutomata 인스턴스
 * @param extra_flags 추가 ImGuiInputTextFlags
 * @return 텍스트 내용이 변경되었으면 true
 */
bool HangulInputText(const char* label, const char* hint,
                     char* buf, size_t buf_size,
                     HangulAutomata& automata,
                     int extra_flags = 0);

} // namespace San13::UI
