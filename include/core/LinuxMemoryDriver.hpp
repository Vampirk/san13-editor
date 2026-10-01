#pragma once

#include "core/IMemoryDriver.hpp"
#include <sys/types.h>
#include <string>

namespace San13::Core {

/**
 * @brief Linux 호스트 환경 전용 메모리 드라이버 (process_vm_readv / writev 시스템 콜 기반)
 * Proton / Wine 가상화 프로세스 메모리에 직접 고속 접근합니다.
 */
class LinuxMemoryDriver : public IMemoryDriver {
public:
    LinuxMemoryDriver();
    virtual ~LinuxMemoryDriver() override;

    bool Attach(const std::string& processName) override;
    void Detach() override;
    bool IsAlive() override;
    int GetPid() const override;
    uintptr_t GetModuleBase(const std::string& moduleName = "") override;

    bool ReadRaw(uintptr_t remoteAddr, void* localBuf, size_t size) override;
    bool WriteRaw(uintptr_t remoteAddr, const void* localBuf, size_t size) override;

private:
    pid_t m_pid;
    std::string m_processName;
    uintptr_t m_baseAddr;

    static pid_t FindPidByName(const std::string& name);
    static uintptr_t ParseMapsForBase(pid_t pid, const std::string& moduleName);
};

} // namespace San13::Core
