#include "core/Win32MemoryDriver.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <strings.h>
#include <cstring>

namespace San13::Core {

Win32MemoryDriver::Win32MemoryDriver()
    : m_handle(nullptr), m_pid(0), m_baseAddr(0) {
}

Win32MemoryDriver::~Win32MemoryDriver() {
    Detach();
}

bool Win32MemoryDriver::Attach(const std::string& processName) {
    Detach();

    m_pid = FindPidByName(processName);
    if (m_pid == 0) return false;

    HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
                           PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
                           FALSE, m_pid);
    if (!h) {
        m_pid = 0;
        return false;
    }
    m_handle = h;
    m_processName = processName;
    m_baseAddr = FindModuleBase(m_pid, processName);
    if (m_baseAddr == 0) {
        Detach();
        return false;
    }
    return true;
}

void Win32MemoryDriver::Detach() {
    if (m_handle) {
        CloseHandle(static_cast<HANDLE>(m_handle));
    }
    m_handle = nullptr;
    m_pid = 0;
    m_baseAddr = 0;
    m_processName.clear();
}

bool Win32MemoryDriver::IsAlive() {
    if (!m_handle) return false;
    // 시그널 상태가 아니면(WAIT_TIMEOUT) 프로세스가 살아있음
    if (WaitForSingleObject(static_cast<HANDLE>(m_handle), 0) != WAIT_TIMEOUT) {
        Detach();
        return false;
    }
    return true;
}

int Win32MemoryDriver::GetPid() const {
    return m_handle ? static_cast<int>(m_pid) : -1;
}

uintptr_t Win32MemoryDriver::GetModuleBase(const std::string& moduleName) {
    if (!IsAlive()) return 0;
    if (moduleName.empty() || strcasecmp(moduleName.c_str(), m_processName.c_str()) == 0) {
        if (m_baseAddr == 0) m_baseAddr = FindModuleBase(m_pid, m_processName);
        return m_baseAddr;
    }
    return FindModuleBase(m_pid, moduleName);
}

bool Win32MemoryDriver::ReadRaw(uintptr_t remoteAddr, void* localBuf, size_t size) {
    if (!m_handle || remoteAddr == 0 || !localBuf || size == 0) return false;
    SIZE_T n = 0;
    if (!ReadProcessMemory(static_cast<HANDLE>(m_handle),
                           reinterpret_cast<LPCVOID>(remoteAddr), localBuf, size, &n)) {
        if (GetLastError() == ERROR_INVALID_HANDLE) Detach();
        return false;
    }
    return n == size;
}

bool Win32MemoryDriver::WriteRaw(uintptr_t remoteAddr, const void* localBuf, size_t size) {
    if (!m_handle || remoteAddr == 0 || !localBuf || size == 0) return false;
    SIZE_T n = 0;
    if (!WriteProcessMemory(static_cast<HANDLE>(m_handle),
                            reinterpret_cast<LPVOID>(remoteAddr), localBuf, size, &n)) {
        if (GetLastError() == ERROR_INVALID_HANDLE) Detach();
        return false;
    }
    return n == size;
}

unsigned long Win32MemoryDriver::FindPidByName(const std::string& name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    unsigned long found = 0;
    if (Process32First(snap, &pe)) {
        do {
            if (strcasecmp(pe.szExeFile, name.c_str()) == 0) {
                found = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

uintptr_t Win32MemoryDriver::FindModuleBase(unsigned long pid, const std::string& moduleName) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return 0;

    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    uintptr_t base = 0;

    // WoW64 환경에서 32비트 프로세스 모듈을 64비트 프로세스가 안전하게 조회하도록 LIST_MODULES_ALL 지정
    if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL)) {
        size_t count = cbNeeded / sizeof(HMODULE);
        for (size_t i = 0; i < count; ++i) {
            char szModName[MAX_PATH];
            if (GetModuleBaseNameA(hProcess, hMods[i], szModName, sizeof(szModName))) {
                if (moduleName.empty() || strcasecmp(szModName, moduleName.c_str()) == 0) {
                    base = reinterpret_cast<uintptr_t>(hMods[i]);
                    break;
                }
            }
        }
    }

    CloseHandle(hProcess);
    return base;
}

} // namespace San13::Core
