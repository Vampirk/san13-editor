#pragma once

#include "core/IMemoryDriver.hpp"
#include <string>

namespace San13::Core {

/**
 * @brief Windows 전용 메모리 드라이버 (OpenProcess / ReadProcessMemory / WriteProcessMemory 기반)
 * 핸들은 void*로 보관하여 헤더에서 windows.h 노출을 피한다.
 */
class Win32MemoryDriver : public IMemoryDriver {
public:
    Win32MemoryDriver();
    virtual ~Win32MemoryDriver() override;

    bool Attach(const std::string& processName) override;
    void Detach() override;
    bool IsAlive() override;
    int GetPid() const override;
    uintptr_t GetModuleBase(const std::string& moduleName = "") override;

    bool ReadRaw(uintptr_t remoteAddr, void* localBuf, size_t size) override;
    bool WriteRaw(uintptr_t remoteAddr, const void* localBuf, size_t size) override;

private:
    void* m_handle;
    unsigned long m_pid;
    std::string m_processName;
    uintptr_t m_baseAddr;

    static unsigned long FindPidByName(const std::string& name);
    static uintptr_t FindModuleBase(unsigned long pid, const std::string& moduleName);
};

} // namespace San13::Core
