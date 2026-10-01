#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <type_traits>

namespace San13::Core {

/**
 * @brief OS 추상화 메모리 드라이버 인터페이스 (Linux process_vm vs Win32 ReadProcessMemory)
 */
class IMemoryDriver {
public:
    virtual ~IMemoryDriver() = default;

    /**
     * @brief 프로세스 이름으로 타깃 프로세스 탐색 및 부착 (Attach)
     * @param processName 타깃 실행 파일 이름 (예: "San13PK_kr.exe")
     * @return 성공 여부 (미실행 시 false 반환, 크래시 없음)
     */
    virtual bool Attach(const std::string& processName) = 0;

    /**
     * @brief 프로세스 분리 및 리소스 해제
     */
    virtual void Detach() = 0;

    /**
     * @brief 현재 연결된 프로세스가 실제로 살아있는지 확인
     * @return 생존 여부 (도중 종료 시 false)
     */
    virtual bool IsAlive() = 0;

    /**
     * @brief 현재 부착된 프로세스 ID(PID) 반환
     */
    virtual int GetPid() const = 0;

    /**
     * @brief 프로세스 메인 모듈 또는 특정 모듈의 가상 메모리 베이스 주소 획득
     * @param moduleName 모듈 이름 (비어있을 시 메인 바이너리)
     * @return 베이스 주소 (실패 시 0)
     */
    virtual uintptr_t GetModuleBase(const std::string& moduleName = "") = 0;

    /**
     * @brief 원시 바이트 블록 메모리 읽기
     */
    virtual bool ReadRaw(uintptr_t remoteAddr, void* localBuf, size_t size) = 0;

    /**
     * @brief 원시 바이트 블록 메모리 쓰기
     */
    virtual bool WriteRaw(uintptr_t remoteAddr, const void* localBuf, size_t size) = 0;

    /**
     * @brief 타입 기반 안전 메모리 읽기 템플릿
     */
    template <typename T>
    bool Read(uintptr_t remoteAddr, T& outVal) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memory read");
        return ReadRaw(remoteAddr, &outVal, sizeof(T));
    }

    /**
     * @brief 타입 기반 안전 메모리 쓰기 템플릿
     */
    template <typename T>
    bool Write(uintptr_t remoteAddr, const T& inVal) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memory write");
        return WriteRaw(remoteAddr, &inVal, sizeof(T));
    }
};

} // namespace San13::Core
