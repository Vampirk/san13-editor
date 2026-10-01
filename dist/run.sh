#!/usr/bin/env bash
# ==============================================================================
#  삼국지 13 PK 한국어 메모리 에디터 & 트레이너 (San13PK Korean Suite)
#  Linux Standalone Execution Wrapper Script
# ==============================================================================

set -e

# 스크립트가 위치한 절대 디렉터리로 이동
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BINARY="./san13_editor"

if [ ! -f "$BINARY" ]; then
    echo "[-] Error: 실행 바이너리 ($BINARY)를 찾을 수 없습니다."
    exit 1
fi

# 실행 권한 부여
chmod +x "$BINARY"

# 1. Linux ptrace_scope 보안 설정 검사
if [ -f "/proc/sys/kernel/yama/ptrace_scope" ]; then
    PTRACE_SCOPE=$(cat /proc/sys/kernel/yama/ptrace_scope 2>/dev/null || echo "1")
    if [ "$PTRACE_SCOPE" -gt 1 ]; then
        echo "[!] 주의: Linux Yama ptrace_scope가 $PTRACE_SCOPE 로 설정되어 있어 타 프로세스 메모리 접근이 차단될 수 있습니다."
        echo "    권한 오류가 발생할 경우 아래 명령어를 한 번 실행해 주세요:"
        echo "    sudo sysctl -w kernel.yama.ptrace_scope=1"
        echo ""
    fi
fi

# 2. Wayland / X11 그래픽 디스플레이 환경 검사
if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
    echo "[-] Error: GUI 디스플레이 환경(X11/Wayland)을 감지할 수 없습니다."
    exit 1
fi

echo "============================================================"
echo " 삼국지 13 PK 한국어 메모리 에디터 & 트레이너를 시작합니다..."
echo "============================================================"

# 바이너리 실행
exec "$BINARY" "$@"
