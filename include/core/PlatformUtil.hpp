#pragma once

#include <string>
#include <cstddef>

namespace San13::Core {

/**
 * @brief 플랫폼 독립 유틸리티 (Linux: iconv/access, Windows: MultiByteToWideChar/GetFileAttributes)
 */

/// CP949(EUC-KR 확장) 바이트열을 UTF-8로 변환. NUL 또는 maxLen에서 종료. 실패 시 원본 바이트 반환.
std::string Cp949ToUtf8(const char* data, size_t maxLen);

/// 파일이 존재하고 읽기 가능한지 검사.
bool IsFileReadable(const std::string& path);

/// 사용자 홈 디렉터리 (Linux: $HOME, Windows: %USERPROFILE%). 없으면 빈 문자열.
std::string GetHomeDir();

} // namespace San13::Core
