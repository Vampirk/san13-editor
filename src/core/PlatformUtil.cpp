#include "core/PlatformUtil.hpp"

#include <cstdlib>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#include <unistd.h>
#endif

namespace San13::Core {

std::string Cp949ToUtf8(const char* data, size_t maxLen) {
    if (!data || maxLen == 0 || data[0] == '\0') return "";
    size_t inLen = 0;
    while (inLen < maxLen && data[inLen] != '\0') inLen++;
    if (inLen == 0) return "";

#ifdef _WIN32
    int wlen = MultiByteToWideChar(949, 0, data, static_cast<int>(inLen), nullptr, 0);
    if (wlen <= 0) return std::string(data, inLen);
    std::vector<wchar_t> wbuf(static_cast<size_t>(wlen));
    MultiByteToWideChar(949, 0, data, static_cast<int>(inLen), wbuf.data(), wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), wlen, nullptr, 0, nullptr, nullptr);
    if (ulen <= 0) return std::string(data, inLen);
    std::string out(static_cast<size_t>(ulen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), wlen, &out[0], ulen, nullptr, nullptr);
    return out;
#else
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
#endif
}

bool IsFileReadable(const std::string& path) {
    if (path.empty()) return false;
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    return access(path.c_str(), R_OK) == 0;
#endif
}

std::string GetHomeDir() {
#ifdef _WIN32
    const char* p = std::getenv("USERPROFILE");
#else
    const char* p = std::getenv("HOME");
#endif
    return p ? std::string(p) : std::string();
}

} // namespace San13::Core
