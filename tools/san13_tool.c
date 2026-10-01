#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/uio.h>
#include <unistd.h>

// Version mapping table based on Van Editor reverse engineering
typedef struct {
    uint32_t timedatestamp;
    uint32_t version_id;
    uint32_t global_offset;
    const char* desc;
} GameVersionInfo;

static const GameVersionInfo VERSION_TABLE[] = {
    { 0x59919494, 0x33, 0x01FB94A8, "Korean PK 1.0.0" },
    { 0x5D834FFA, 0x34, 0x01FBC618, "Korean PK 1.0.1" },
    { 0x5E3BA719, 0x35, 0x01FBC5D8, "Korean PK 1.0.2 (Latest)" },
    { 0x582C515D, 0x20, 0x01F984E8, "Japanese PK 1.0.0" },
    { 0x5E3BA678, 0x2A, 0x02DF1288, "Japanese PK Latest" },
    { 0x582C529D, 0x2B, 0x01F93F08, "Traditional Chinese PK 1.0.0" },
    { 0x5E3BA5D5, 0x32, 0x01FCAA18, "Traditional Chinese PK Latest" },
    { 0x58F9BBC5, 0x36, 0x0204CFD0, "English PK 1.0.0" },
    { 0x5E3BA865, 0x39, 0x02052110, "English PK Latest" },
    { 0, 0, 0, NULL }
};

// Known famous officers table for display
typedef struct {
    uint16_t id;
    const char* name;
} OfficerMeta;

static const OfficerMeta FAMOUS_OFFICERS[] = {
    { 94,  "관우 (Guan Yu)" },
    { 137, "허저 (Xu Chu)" },
    { 178, "전위 (Dian Wei)" },
    { 220, "사마의 (Sima Yi)" },
    { 241, "주유 (Zhou Yu)" },
    { 283, "제갈량 (Zhuge Liang)" },
    { 333, "조조 (Cao Cao)" },
    { 359, "손책 (Sun Ce)" },
    { 380, "조운 (Zhao Yun)" },
    { 418, "장비 (Zhang Fei)" },
    { 458, "안량 (Yan Liang)" },
    { 498, "문추 (Wen Chou)" },
    { 635, "여포 (Lu Bu)" },
    { 672, "유비 (Liu Bei)" },
    { 700, "항우 (Xiang Yu)" }
};

// Memory read helper via process_vm_readv
bool read_remote(pid_t pid, uintptr_t remote_addr, void* local_buf, size_t size) {
    struct iovec local = { local_buf, size };
    struct iovec remote = { (void*)remote_addr, size };
    ssize_t nread = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    return (nread == (ssize_t)size);
}

// Memory write helper via process_vm_writev
bool write_remote(pid_t pid, uintptr_t remote_addr, const void* local_buf, size_t size) {
    struct iovec local = { (void*)local_buf, size };
    struct iovec remote = { (void*)remote_addr, size };
    ssize_t nwritten = process_vm_writev(pid, &local, 1, &remote, 1, 0);
    return (nwritten == (ssize_t)size);
}

// Find PID by scanning /proc/*/cmdline
pid_t find_san13_pid(void) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent* entry;
    pid_t found_pid = -1;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;
        char* endptr;
        long id = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue;

        char path[256];
        snprintf(path, sizeof(path), "/proc/%ld/cmdline", id);
        FILE* fp = fopen(path, "rb");
        if (fp) {
            char buf[512];
            size_t len = fread(buf, 1, sizeof(buf) - 1, fp);
            fclose(fp);
            if (len > 0) {
                buf[len] = '\0';
                if (strcasestr(buf, "san13pk") != NULL) {
                    found_pid = (pid_t)id;
                    break;
                }
            }
        }
    }
    closedir(dir);
    return found_pid;
}

// Find module base address by parsing /proc/<pid>/maps
uintptr_t get_module_base(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* fp = fopen(path, "r");
    if (!fp) return 0;

    char line[1024];
    uintptr_t base_addr = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strcasestr(line, "san13pk") != NULL) {
            uintptr_t start, end;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                base_addr = start;
                break;
            }
        }
    }
    fclose(fp);
    return base_addr;
}

#pragma pack(push, 1)
typedef struct {
    uint8_t  base_leadership;
    uint8_t  base_war;
    uint8_t  base_intel;
    uint8_t  base_politics;
    uint8_t  curr_leadership;
    uint8_t  curr_war;
    uint8_t  curr_intel;
    uint8_t  curr_politics;
    uint8_t  exp_leadership;
    uint8_t  exp_war;
    uint8_t  exp_intel;
    uint8_t  exp_politics;
    uint8_t  spear_apt;
    uint8_t  horse_apt;
    uint8_t  bow_apt;
    uint8_t  unknown_0f;
} OfficerLiveStats;
#pragma pack(pop)

int main(int argc, char* argv[]) {
    printf("====================================================================\n");
    printf(" [San13PK Modern Memory Core - Phase 3 Real-Time Read/Write Engine]\n");
    printf("====================================================================\n\n");

    pid_t pid = find_san13_pid();
    if (pid <= 0) {
        fprintf(stderr, "[-] Error: San13PK process not found in /proc!\n");
        return 1;
    }

    uintptr_t base_addr = get_module_base(pid);
    if (!base_addr) {
        fprintf(stderr, "[-] Error: Could not resolve San13PK base address!\n");
        return 1;
    }

    uint8_t dos_header[64];
    if (!read_remote(pid, base_addr, dos_header, sizeof(dos_header)) || dos_header[0] != 'M' || dos_header[1] != 'Z') {
        fprintf(stderr, "[-] Error: Failed to read valid DOS header at 0x%lx\n", base_addr);
        return 1;
    }

    uint32_t e_lfanew = *(uint32_t*)&dos_header[0x3C];
    uint8_t nt_header[24];
    if (!read_remote(pid, base_addr + e_lfanew, nt_header, sizeof(nt_header))) {
        fprintf(stderr, "[-] Error: Failed to read NT header!\n");
        return 1;
    }

    uint32_t timedatestamp = *(uint32_t*)&nt_header[8];
    const GameVersionInfo* ver = NULL;
    for (int i = 0; VERSION_TABLE[i].desc != NULL; i++) {
        if (VERSION_TABLE[i].timedatestamp == timedatestamp) {
            ver = &VERSION_TABLE[i];
            break;
        }
    }

    if (!ver) {
        fprintf(stderr, "[-] Error: Unsupported TimeDateStamp 0x%08X\n", timedatestamp);
        return 1;
    }

    uintptr_t root_base = base_addr + ver->global_offset;
    uintptr_t officer_array_base = root_base + 0x015EE0;
    const size_t OFFICER_STRIDE = 0x12BC;

    printf("[+] Game: %s (PID: %d)\n", ver->desc, pid);
    printf("[+] Module Base: 0x%lx | RootBase: 0x%lx\n", base_addr, root_base);
    printf("[+] Officer Pool Base: 0x%lx (Stride: 0x%zx = %zu bytes)\n\n", 
           officer_array_base, OFFICER_STRIDE, OFFICER_STRIDE);

    // Command line arguments for writing
    if (argc >= 4 && strcmp(argv[1], "set") == 0) {
        uint16_t off_id = (uint16_t)atoi(argv[2]);
        uint8_t new_war = (uint8_t)atoi(argv[3]);

        uintptr_t obj_addr = officer_array_base + (off_id * OFFICER_STRIDE);
        uintptr_t stats_addr = obj_addr + 0x15;

        OfficerLiveStats s;
        if (!read_remote(pid, stats_addr, &s, sizeof(s))) {
            fprintf(stderr, "[-] Failed to read officer %d stats\n", off_id);
            return 1;
        }

        printf("[!] [WRITE ACTION] Modifying Officer ID %d:\n", off_id);
        printf("    Before: Base War = %d, Current War = %d\n", s.base_war, s.curr_war);

        s.base_war = new_war;
        s.curr_war = new_war;

        if (write_remote(pid, stats_addr, &s, sizeof(s))) {
            printf("[+] [SUCCESS] Written new War = %d to memory at 0x%lx\n", new_war, stats_addr);
        } else {
            fprintf(stderr, "[-] Failed to write stats to memory!\n");
            return 1;
        }

        // Verify read back
        OfficerLiveStats v;
        read_remote(pid, stats_addr, &v, sizeof(v));
        printf("    Verified After: Base War = %d, Current War = %d\n\n", v.base_war, v.curr_war);
        return 0;
    }

    // Default: Display Famous Officers
    printf("┌──────┬──────────────────────┬──────┬──────┬──────┬──────┬─────────┬──────────────┐\n");
    printf("│  ID  │ 무장 이름 (Officer)  │ 통솔 │ 무력 │ 지력 │ 정치 │ 병종적성│ 메모리 주소  │\n");
    printf("├──────┼──────────────────────┼──────┼──────┼──────┼──────┼─────────┼──────────────┤\n");

    const char* apt_str[] = { "C", "B", "A", "S" };
    size_t num_famous = sizeof(FAMOUS_OFFICERS) / sizeof(FAMOUS_OFFICERS[0]);

    for (size_t i = 0; i < num_famous; i++) {
        uint16_t id = FAMOUS_OFFICERS[i].id;
        uintptr_t obj_addr = officer_array_base + (id * OFFICER_STRIDE);
        uintptr_t stats_addr = obj_addr + 0x15;

        OfficerLiveStats s;
        if (read_remote(pid, stats_addr, &s, sizeof(s))) {
            printf("│ %4d │ %-20s │  %3d │  %3d │  %3d │  %3d │ %s / %s / %s │ 0x%08lx │\n",
                   id, FAMOUS_OFFICERS[i].name,
                   s.base_war == 99 && id == 333 ? s.base_leadership : s.base_leadership,
                   s.base_war, s.base_intel, s.base_politics,
                   apt_str[s.spear_apt & 3], apt_str[s.horse_apt & 3], apt_str[s.bow_apt & 3],
                   stats_addr);
        }
    }
    printf("└──────┴──────────────────────┴──────┴──────┴──────┴──────┴─────────┴──────────────┘\n\n");

    printf("[*] 사용법 (무력 수정):\n");
    printf("    %s set <OfficerID> <NewWarValue>\n", argv[0]);
    printf("    예시 (조조 무력 원복/수정): %s set 333 72\n\n", argv[0]);

    return 0;
}
