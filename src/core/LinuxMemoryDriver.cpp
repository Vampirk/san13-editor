#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "core/LinuxMemoryDriver.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <sys/uio.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>

namespace San13::Core {

LinuxMemoryDriver::LinuxMemoryDriver()
    : m_pid(-1), m_baseAddr(0) {
}

LinuxMemoryDriver::~LinuxMemoryDriver() {
    Detach();
}

bool LinuxMemoryDriver::Attach(const std::string& processName) {
    Detach();

    m_pid = FindPidByName(processName);
    if (m_pid <= 0) {
        // EDGE-09: 게임 미실행 상태에서 비정상 크래시 없이 안전하게 false 반환
        return false;
    }

    m_processName = processName;
    m_baseAddr = ParseMapsForBase(m_pid, processName);

    return (m_baseAddr != 0);
}

void LinuxMemoryDriver::Detach() {
    m_pid = -1;
    m_baseAddr = 0;
    m_processName.clear();
}

bool LinuxMemoryDriver::IsAlive() {
    if (m_pid <= 0) return false;

    // kill(pid, 0)으로 프로세스 존재 여부 검사
    if (kill(m_pid, 0) != 0) {
        // EDGE-10: 프로세스 비정상 종료 감지 시 드라이버 상태 자동 정리
        Detach();
        return false;
    }

    // /proc/<pid>/cmdline 존재 여부 교차 검증 (Zombified 프로세스 방지)
    std::string cmdPath = "/proc/" + std::to_string(m_pid) + "/cmdline";
    if (access(cmdPath.c_str(), F_OK) != 0) {
        Detach();
        return false;
    }

    return true;
}

int LinuxMemoryDriver::GetPid() const {
    return static_cast<int>(m_pid);
}

uintptr_t LinuxMemoryDriver::GetModuleBase(const std::string& moduleName) {
    if (!IsAlive()) return 0;

    if (moduleName.empty() || strcasecmp(moduleName.c_str(), m_processName.c_str()) == 0) {
        if (m_baseAddr == 0) {
            m_baseAddr = ParseMapsForBase(m_pid, m_processName);
        }
        return m_baseAddr;
    }

    return ParseMapsForBase(m_pid, moduleName);
}

bool LinuxMemoryDriver::ReadRaw(uintptr_t remoteAddr, void* localBuf, size_t size) {
    if (m_pid <= 0 || remoteAddr == 0 || localBuf == nullptr || size == 0) {
        return false;
    }

    struct iovec local = { localBuf, size };
    struct iovec remote = { reinterpret_cast<void*>(remoteAddr), size };

    ssize_t nread = process_vm_readv(m_pid, &local, 1, &remote, 1, 0);
    if (nread < 0) {
        // EDGE-10: 읽기 도중 프로세스가 죽었는지 검사
        if (errno == ESRCH) {
            Detach();
        }
        return false;
    }

    return (nread == static_cast<ssize_t>(size));
}

bool LinuxMemoryDriver::WriteRaw(uintptr_t remoteAddr, const void* localBuf, size_t size) {
    if (m_pid <= 0 || remoteAddr == 0 || localBuf == nullptr || size == 0) {
        return false;
    }

    struct iovec local = { const_cast<void*>(localBuf), size };
    struct iovec remote = { reinterpret_cast<void*>(remoteAddr), size };

    ssize_t nwritten = process_vm_writev(m_pid, &local, 1, &remote, 1, 0);
    if (nwritten < 0) {
        if (errno == ESRCH) {
            Detach();
        }
        return false;
    }

    return (nwritten == static_cast<ssize_t>(size));
}

pid_t LinuxMemoryDriver::FindPidByName(const std::string& name) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent* entry;
    pid_t found_pid = -1;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;

        char* endptr;
        long id = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue;

        std::string cmdPath = std::string("/proc/") + entry->d_name + "/cmdline";
        FILE* fp = fopen(cmdPath.c_str(), "rb");
        if (fp) {
            char buf[512];
            size_t len = fread(buf, 1, sizeof(buf) - 1, fp);
            fclose(fp);

            if (len > 0) {
                buf[len] = '\0';
                // 대소문자 구분 없이 타깃 이름 검색
                if (strcasestr(buf, name.c_str()) != nullptr) {
                    found_pid = static_cast<pid_t>(id);
                    break;
                }
            }
        }
    }

    closedir(dir);
    return found_pid;
}

uintptr_t LinuxMemoryDriver::ParseMapsForBase(pid_t pid, const std::string& moduleName) {
    std::string mapsPath = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream file(mapsPath);
    if (!file.is_open()) return 0;

    std::string line;
    while (std::getline(file, line)) {
        if (strcasestr(line.c_str(), moduleName.c_str()) != nullptr) {
            // First matching memory region
            uintptr_t start = 0, end = 0;
            char perms[5] = {0};
            if (sscanf(line.c_str(), "%lx-%lx %4s", &start, &end, perms) >= 2) {
                // Return base address of the module
                return start;
            }
        }
    }

    return 0;
}

} // namespace San13::Core
