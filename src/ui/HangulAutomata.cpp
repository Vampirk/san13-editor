#include "ui/HangulAutomata.hpp"
#include "imgui.h"
#include <cctype>
#include <cstring>

namespace San13::UI {

namespace {

// 초성 19자 호환용 자모(Compatibility Jamo) 유니코드 매핑 (0x3131 ~ 0x314E)
static const uint32_t CHO_TO_COMPAT[19] = {
    0x3131, // 0: ㄱ
    0x3132, // 1: ㄲ
    0x3134, // 2: ㄴ
    0x3137, // 3: ㄷ
    0x3138, // 4: ㄸ
    0x3139, // 5: ㄹ
    0x3141, // 6: ㅁ
    0x3142, // 7: ㅂ
    0x3143, // 8: ㅃ
    0x3145, // 9: ㅅ
    0x3146, // 10: ㅆ
    0x3147, // 11: ㅇ
    0x3148, // 12: ㅈ
    0x3149, // 13: ㅉ
    0x314A, // 14: ㅊ
    0x314B, // 15: ㅋ
    0x314C, // 16: ㅌ
    0x314D, // 17: ㅍ
    0x314E  // 18: ㅎ
};

enum class JamoKind { None, Consonant, Vowel };

struct KeyJamoMapping {
    JamoKind kind = JamoKind::None;
    int cho = -1;       // 초성 인덱스 (0..18)
    int jung = -1;      // 중성 인덱스 (0..20)
    int jong = -1;      // 종성 인덱스 (1..27, 불가는 -1)
};

static KeyJamoMapping MapKeyToJamo(char c) {
    KeyJamoMapping m;
    switch (c) {
        // 자음 (Consonants)
        case 'r': m.kind = JamoKind::Consonant; m.cho = 0;  m.jong = 1;  break; // ㄱ
        case 'R': m.kind = JamoKind::Consonant; m.cho = 1;  m.jong = 2;  break; // ㄲ
        case 's':
        case 'S': m.kind = JamoKind::Consonant; m.cho = 2;  m.jong = 4;  break; // ㄴ
        case 'e': m.kind = JamoKind::Consonant; m.cho = 3;  m.jong = 7;  break; // ㄷ
        case 'E': m.kind = JamoKind::Consonant; m.cho = 4;  m.jong = -1; break; // ㄸ (종성 불가)
        case 'f':
        case 'F': m.kind = JamoKind::Consonant; m.cho = 5;  m.jong = 8;  break; // ㄹ
        case 'a':
        case 'A': m.kind = JamoKind::Consonant; m.cho = 6;  m.jong = 16; break; // ㅁ
        case 'q': m.kind = JamoKind::Consonant; m.cho = 7;  m.jong = 17; break; // ㅂ
        case 'Q': m.kind = JamoKind::Consonant; m.cho = 8;  m.jong = -1; break; // ㅃ (종성 불가)
        case 't': m.kind = JamoKind::Consonant; m.cho = 9;  m.jong = 19; break; // ㅅ
        case 'T': m.kind = JamoKind::Consonant; m.cho = 10; m.jong = 20; break; // ㅆ
        case 'd':
        case 'D': m.kind = JamoKind::Consonant; m.cho = 11; m.jong = 21; break; // ㅇ
        case 'w': m.kind = JamoKind::Consonant; m.cho = 12; m.jong = 22; break; // ㅈ
        case 'W': m.kind = JamoKind::Consonant; m.cho = 13; m.jong = -1; break; // ㅉ (종성 불가)
        case 'c':
        case 'C': m.kind = JamoKind::Consonant; m.cho = 14; m.jong = 23; break; // ㅊ
        case 'z':
        case 'Z': m.kind = JamoKind::Consonant; m.cho = 15; m.jong = 24; break; // ㅋ
        case 'x':
        case 'X': m.kind = JamoKind::Consonant; m.cho = 16; m.jong = 25; break; // ㅌ
        case 'v':
        case 'V': m.kind = JamoKind::Consonant; m.cho = 17; m.jong = 26; break; // ㅍ
        case 'g':
        case 'G': m.kind = JamoKind::Consonant; m.cho = 18; m.jong = 27; break; // ㅎ

        // 모음 (Vowels)
        case 'k':
        case 'K': m.kind = JamoKind::Vowel; m.jung = 0;  break; // ㅏ
        case 'o': m.kind = JamoKind::Vowel; m.jung = 1;  break; // ㅐ
        case 'i':
        case 'I': m.kind = JamoKind::Vowel; m.jung = 2;  break; // ㅑ
        case 'O': m.kind = JamoKind::Vowel; m.jung = 3;  break; // ㅒ
        case 'j':
        case 'J': m.kind = JamoKind::Vowel; m.jung = 4;  break; // ㅓ
        case 'p': m.kind = JamoKind::Vowel; m.jung = 5;  break; // ㅔ
        case 'u':
        case 'U': m.kind = JamoKind::Vowel; m.jung = 6;  break; // ㅕ
        case 'P': m.kind = JamoKind::Vowel; m.jung = 7;  break; // ㅖ
        case 'h':
        case 'H': m.kind = JamoKind::Vowel; m.jung = 8;  break; // ㅗ
        case 'y':
        case 'Y': m.kind = JamoKind::Vowel; m.jung = 12; break; // ㅛ
        case 'n':
        case 'N': m.kind = JamoKind::Vowel; m.jung = 13; break; // ㅜ
        case 'b':
        case 'B': m.kind = JamoKind::Vowel; m.jung = 17; break; // ㅠ
        case 'm':
        case 'M': m.kind = JamoKind::Vowel; m.jung = 18; break; // ㅡ
        case 'l':
        case 'L': m.kind = JamoKind::Vowel; m.jung = 20; break; // ㅣ

        default:
            m.kind = JamoKind::None;
            break;
    }
    return m;
}

// 복모음 조합 (7종)
static int CombineJung(int first, int second) {
    if (first == 8) { // ㅗ
        if (second == 0)  return 9;  // ㅗ + ㅏ = ㅘ
        if (second == 1)  return 10; // ㅗ + ㅐ = ㅙ
        if (second == 20) return 11; // ㅗ + ㅣ = ㅚ
    } else if (first == 13) { // ㅜ
        if (second == 4)  return 14; // ㅜ + ㅓ = ㅝ
        if (second == 5)  return 15; // ㅜ + ㅔ = ㅞ
        if (second == 20) return 16; // ㅜ + ㅣ = ㅟ
    } else if (first == 18) { // ㅡ
        if (second == 20) return 19; // ㅡ + ㅣ = ㅢ
    }
    return -1;
}

// 복합 종성 조합 (11종)
static int CombineJong(int first, int second) {
    if (first == 1) { // ㄱ
        if (second == 19) return 3;  // ㄱ + ㅅ = ㄳ
    } else if (first == 4) { // ㄴ
        if (second == 22) return 5;  // ㄴ + ㅈ = ㄵ
        if (second == 27) return 6;  // ㄴ + ㅎ = ㄶ
    } else if (first == 8) { // ㄹ
        if (second == 1)  return 9;  // ㄹ + ㄱ = ㄺ
        if (second == 16) return 10; // ㄹ + ㅁ = ㄻ
        if (second == 17) return 11; // ㄹ + ㅂ = ㄼ
        if (second == 19) return 12; // ㄹ + ㅅ = ㄽ
        if (second == 25) return 13; // ㄹ + ㅌ = ㄾ
        if (second == 26) return 14; // ㄹ + ㅍ = ㄿ
        if (second == 27) return 15; // ㄹ + ㅎ = ㅀ
    } else if (first == 17) { // ㅂ
        if (second == 19) return 18; // ㅂ + ㅅ = ㅄ
    }
    return -1;
}

// 종성 인덱스를 초성 인덱스로 변환 (연음 분리시 사용)
static int JongToCho(int jong) {
    switch (jong) {
        case 1:  return 0;  // ㄱ
        case 2:  return 1;  // ㄲ
        case 4:  return 2;  // ㄴ
        case 7:  return 3;  // ㄷ
        case 8:  return 5;  // ㄹ
        case 16: return 6;  // ㅁ
        case 17: return 7;  // ㅂ
        case 19: return 9;  // ㅅ
        case 20: return 10; // ㅆ
        case 21: return 11; // ㅇ
        case 22: return 12; // ㅈ
        case 23: return 14; // ㅊ
        case 24: return 15; // ㅋ
        case 25: return 16; // ㅌ
        case 26: return 17; // ㅍ
        case 27: return 18; // ㅎ
        default: return -1;
    }
}

} // anonymous namespace

HangulAutomata::HangulAutomata() {
    ResetComposing();
}

void HangulAutomata::SetHangulMode(bool hangul) {
    if (m_hangulMode != hangul) {
        Commit();
        m_hangulMode = hangul;
    }
}

void HangulAutomata::ResetComposing() {
    m_state = AutomataState::Empty;
    m_cho = -1;
    m_jung = -1;
    m_jong = 0;
    m_jung1 = -1;
    m_jung2 = -1;
    m_jong1 = 0;
    m_jong2 = 0;
}

void HangulAutomata::Clear() {
    m_committedText.clear();
    ResetComposing();
}

void HangulAutomata::SetText(const std::string& text) {
    if (m_committedText == text && m_state == AutomataState::Empty) {
        return;
    }
    m_committedText = text;
    ResetComposing();
}

void HangulAutomata::Commit() {
    std::string comp = GetComposingSyllable();
    if (!comp.empty()) {
        m_committedText += comp;
    }
    ResetComposing();
}

std::string HangulAutomata::CodepointToUtf8(uint32_t cp) {
    std::string out;
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

void HangulAutomata::PopLastUtf8Char(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size();
    while (i > 0) {
        --i;
        if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
            s.erase(i);
            break;
        }
    }
}

std::string HangulAutomata::GetComposingSyllable() const {
    switch (m_state) {
        case AutomataState::Empty:
            return "";
        case AutomataState::ChoOnly:
            if (m_cho >= 0 && m_cho < 19) {
                return CodepointToUtf8(CHO_TO_COMPAT[m_cho]);
            }
            return "";
        case AutomataState::ChoJung:
        case AutomataState::ChoJungJong:
        case AutomataState::ChoJungDoubleJong:
            if (m_cho >= 0 && m_cho < 19 && m_jung >= 0 && m_jung < 21) {
                uint32_t cp = 0xAC00 + (m_cho * 21 + m_jung) * 28 + m_jong;
                return CodepointToUtf8(cp);
            }
            return "";
    }
    return "";
}

std::string HangulAutomata::GetText() const {
    return m_committedText + GetComposingSyllable();
}

bool HangulAutomata::ProcessKey(char c) {
    // 1. 영문 모드일 경우: 그대로 커밋 버퍼에 추가
    if (!m_hangulMode) {
        m_committedText.push_back(c);
        return true;
    }

    KeyJamoMapping key = MapKeyToJamo(c);

    // 2. 한글 자모 키가 아닌 경우 (공백, 숫자, 특수문자, 줄바꿈 등)
    if (key.kind == JamoKind::None) {
        Commit();
        m_committedText.push_back(c);
        return true;
    }

    // 3. 자음 입력 처리
    if (key.kind == JamoKind::Consonant) {
        switch (m_state) {
            case AutomataState::Empty: {
                m_cho = key.cho;
                m_state = AutomataState::ChoOnly;
                break;
            }
            case AutomataState::ChoOnly: {
                // 초성이 연속 2번 오면 이전 초성을 단독 자음으로 커밋
                m_committedText += CodepointToUtf8(CHO_TO_COMPAT[m_cho]);
                m_cho = key.cho;
                m_state = AutomataState::ChoOnly;
                break;
            }
            case AutomataState::ChoJung: {
                // 받침 가능 여부 확인
                if (key.jong > 0) {
                    m_jong = key.jong;
                    m_jong1 = key.jong;
                    m_jong2 = 0;
                    m_state = AutomataState::ChoJungJong;
                } else {
                    // ㄸ, ㅃ, ㅉ 등 종성 불가 자음은 이전 글자 커밋 후 새 초성 시작
                    Commit();
                    m_cho = key.cho;
                    m_state = AutomataState::ChoOnly;
                }
                break;
            }
            case AutomataState::ChoJungJong: {
                // 복합 받침(ㄺ, ㄳ 등) 결합 시도
                int comb = CombineJong(m_jong1, key.jong);
                if (comb > 0) {
                    m_jong = comb;
                    m_jong2 = key.jong;
                    m_state = AutomataState::ChoJungDoubleJong;
                } else {
                    // 결합 불가한 자음이면 이전 음절 완성 커밋 후 새 초성 시작
                    Commit();
                    m_cho = key.cho;
                    m_state = AutomataState::ChoOnly;
                }
                break;
            }
            case AutomataState::ChoJungDoubleJong: {
                // 이미 복합받침이면 더 이상 자음을 받을 수 없으므로 커밋 후 새 초성
                Commit();
                m_cho = key.cho;
                m_state = AutomataState::ChoOnly;
                break;
            }
        }
        return true;
    }

    // 4. 모음 입력 처리
    if (key.kind == JamoKind::Vowel) {
        switch (m_state) {
            case AutomataState::Empty: {
                // 초성 없는 단독 모음 커밋
                m_committedText += CodepointToUtf8(0x314F + key.jung);
                break;
            }
            case AutomataState::ChoOnly: {
                // 초성 + 모음 결합
                m_jung = key.jung;
                m_jung1 = key.jung;
                m_jung2 = -1;
                m_jong = 0;
                m_state = AutomataState::ChoJung;
                break;
            }
            case AutomataState::ChoJung: {
                // 복모음(ㅘ, ㅝ, ㅢ 등) 결합 시도
                int comb = CombineJung(m_jung1, key.jung);
                if (comb > 0 && m_jung2 == -1) {
                    m_jung = comb;
                    m_jung2 = key.jung;
                    m_state = AutomataState::ChoJung;
                } else {
                    // 결합 불가 모음이면 현재 음절 커밋 후 단독 모음 커밋
                    Commit();
                    m_committedText += CodepointToUtf8(0x314F + key.jung);
                }
                break;
            }
            case AutomataState::ChoJungJong: {
                // ★ 단일 받침 연음 분리 (예: '각' + 'ㅏ' -> '가' + '가')
                // 이전 음절은 종성을 뺀 채 커밋
                uint32_t prevCp = 0xAC00 + (m_cho * 21 + m_jung) * 28;
                m_committedText += CodepointToUtf8(prevCp);

                // 분리된 종성을 새 음절의 초성으로 이전
                m_cho = JongToCho(m_jong1);
                m_jung = key.jung;
                m_jung1 = key.jung;
                m_jung2 = -1;
                m_jong = 0;
                m_jong1 = 0;
                m_jong2 = 0;
                m_state = AutomataState::ChoJung;
                break;
            }
            case AutomataState::ChoJungDoubleJong: {
                // ★ 복합 받침 연음 분리 (예: '닭' + 'ㅏ' -> '달' + '가')
                // 이전 음절은 첫 번째 받침(m_jong1)만 남기고 커밋
                uint32_t prevCp = 0xAC00 + (m_cho * 21 + m_jung) * 28 + m_jong1;
                m_committedText += CodepointToUtf8(prevCp);

                // 두 번째 받침(m_jong2)을 새 음절의 초성으로 이전
                m_cho = JongToCho(m_jong2);
                m_jung = key.jung;
                m_jung1 = key.jung;
                m_jung2 = -1;
                m_jong = 0;
                m_jong1 = 0;
                m_jong2 = 0;
                m_state = AutomataState::ChoJung;
                break;
            }
        }
        return true;
    }

    return false;
}

bool HangulAutomata::ProcessBackspace() {
    switch (m_state) {
        case AutomataState::ChoJungDoubleJong: {
            // 복합 받침 -> 단일 받침 환원 (예: '닭' -> '달')
            m_jong = m_jong1;
            m_jong2 = 0;
            m_state = AutomataState::ChoJungJong;
            return true;
        }
        case AutomataState::ChoJungJong: {
            // 종성 제거 -> 초성+중성 환원 (예: '달' -> '다')
            m_jong = 0;
            m_jong1 = 0;
            m_jong2 = 0;
            m_state = AutomataState::ChoJung;
            return true;
        }
        case AutomataState::ChoJung: {
            // 복모음 분해 -> 단모음 환원 (예: '과' -> '고')
            if (m_jung2 != -1) {
                m_jung = m_jung1;
                m_jung2 = -1;
                m_state = AutomataState::ChoJung;
            } else {
                // 단모음 제거 -> 초성 단독 환원 (예: '고' -> 'ㄱ')
                m_jung = -1;
                m_jung1 = -1;
                m_state = AutomataState::ChoOnly;
            }
            return true;
        }
        case AutomataState::ChoOnly: {
            // 초성 제거 -> 빈 상태
            ResetComposing();
            return true;
        }
        case AutomataState::Empty: {
            // 이미 조합 완료된 문자열의 마지막 UTF-8 문자 1자 삭제
            if (!m_committedText.empty()) {
                PopLastUtf8Char(m_committedText);
                return true;
            }
            return false;
        }
    }
    return false;
}

void HangulAutomata::FeedString(const std::string& qwerty) {
    for (char c : qwerty) {
        ProcessKey(c);
    }
}

std::string HangulAutomata::ConvertQwertyToHangul(const std::string& qwerty) {
    HangulAutomata autoInstance;
    autoInstance.FeedString(qwerty);
    autoInstance.Commit();
    return autoInstance.GetText();
}

namespace {

struct HangulInputCallbackData {
    HangulAutomata* automata = nullptr;
    std::string lastSyncedText;
};

static int HangulInputCallback(ImGuiInputTextCallbackData* data) {
    HangulInputCallbackData* cbData = static_cast<HangulInputCallbackData*>(data->UserData);
    if (!cbData || !cbData->automata) return 0;
    HangulAutomata& aut = *cbData->automata;

    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        // 인쇄 가능한 문자(ASCII 32~126)를 가로채 오토마타로 전달
        if (data->EventChar >= 32 && data->EventChar <= 126) {
            aut.ProcessKey(static_cast<char>(data->EventChar));
            data->EventChar = 0; // ImGui 내부 버퍼 직접 삽입 차단
            return 1;
        }
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
        // 1. 단축키 한/영 모드 전환 (입력창이 포커스를 가진 동안에만 동작)
        bool toggleKey = ImGui::IsKeyPressed(ImGuiKey_RightAlt, false) ||
                         (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_Space, false)) ||
                         (ImGui::IsKeyDown(ImGuiKey_RightShift) && ImGui::IsKeyPressed(ImGuiKey_Space, false));
        if (toggleKey) {
            aut.ToggleHangulMode();
        }

        // 2. 백스페이스 감지
        if (ImGui::IsKeyPressed(ImGuiKey_Backspace, true)) {
            aut.ProcessBackspace();
        }

        // 3. 오토마타 텍스트를 ImGui 버퍼와 실시간 동기화
        std::string currentText = aut.GetText();
        if (cbData->lastSyncedText != currentText || std::string(data->Buf) != currentText) {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, currentText.c_str());
            data->CursorPos = data->BufTextLen;
            cbData->lastSyncedText = currentText;
        }
    }
    return 0;
}

} // anonymous namespace

bool HangulInputText(const char* label, const char* hint,
                     char* buf, size_t buf_size,
                     HangulAutomata& automata,
                     int extra_flags) {
    if (!buf || buf_size == 0) return false;

    // 1. 레이아웃: [텍스트 입력창] + [한/영 토글 뱃지]
    // 호출자가 SetNextItemWidth()로 지정한 너비(예: 260px)를 정확히 존중하여 창 오버플로 방지
    float badgeWidth = 36.0f;
    float totalWidth = ImGui::CalcItemWidth();
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float inputWidth = (totalWidth > badgeWidth + spacing + 10.0f) ? (totalWidth - badgeWidth - spacing) : totalWidth;

    ImGui::SetNextItemWidth(inputWidth);

    HangulInputCallbackData cbData;
    cbData.automata = &automata;
    cbData.lastSyncedText = automata.GetText();

    ImGuiInputTextFlags flags = extra_flags |
                                ImGuiInputTextFlags_CallbackCharFilter |
                                ImGuiInputTextFlags_CallbackAlways;

    std::string prevText = automata.GetText();
    bool valueChanged = ImGui::InputTextWithHint(label, hint, buf, buf_size, flags, HangulInputCallback, &cbData);
    bool isActive = ImGui::IsItemActive();

    // 2. 포커스 아웃 또는 엔터/탭 입력 시 조합 중인 음절 커밋
    if (ImGui::IsItemDeactivated()) {
        automata.Commit();
    }

    // 3. 외부에서 buf가 변경되었거나 리셋된 경우 오토마타와 동기화 (아이템 비활성 시에만 안전 반영)
    if (!isActive && std::string(buf) != automata.GetText()) {
        automata.SetText(buf);
    }

    // 4. 버퍼 최종 동기화
    std::string currentText = automata.GetText();
    if (currentText != prevText) {
        snprintf(buf, buf_size, "%s", currentText.c_str());
        valueChanged = true;
    }

    // 5. 한/영 모드 인디케이터 배지 버튼
    ImGui::SameLine();
    bool isHangul = automata.IsHangulMode();
    if (isHangul) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.50f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.60f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.40f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.28f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.38f, 0.42f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.22f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.80f, 1.0f));
    }

    std::string badgeId = std::string(isHangul ? "한" : "영") + "##badge_" + label;
    if (ImGui::Button(badgeId.c_str(), ImVec2(badgeWidth, 0))) {
        automata.ToggleHangulMode();
        ImGui::SetKeyboardFocusHere(-1); // 이전 위젯(InputText)으로 포커스 자동 복원
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("한/영 입력 모드 전환 (단축키: Right Alt / Shift+Space)");
    }
    ImGui::PopStyleColor(4);

    return valueChanged;
}

} // namespace San13::UI
