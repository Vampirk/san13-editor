#!/usr/bin/env python3
"""
삼국지 13 PK 무장 오브젝트 스냅샷/비교 도구 (RE-01, 읽기 전용)

게임 메모리를 절대 쓰지 않습니다. /proc/<pid>/mem 을 읽어 무장 풀 전체(2,165 × 0x12BC)를
파일로 저장하고, 두 스냅샷 사이에 바뀐 바이트를 오프셋별로 요약합니다.

사용법:
  python3 tools/officer_snapshot.py dump  <out.bin>
  python3 tools/officer_snapshot.py diff  <before.bin> <after.bin> [--ids 333,610] [--min-officers 1]

RootBase는 GameVersion.cpp와 같은 방식(모듈 로드 베이스 + 버전별 Global Base Offset)으로 계산합니다.
"""
import argparse
import os
import struct
import sys
from collections import Counter, defaultdict

PROCESS_NAME = "San13PK_kr.exe"
OFFICER_POOL_OFFSET = 0x015EE0
OFFICER_STRIDE = 0x12BC
OFFICER_COUNT = 2165
DATE_OFFSET = 0x137A
# PE TimeDateStamp → Global Base Offset (src/game/GameVersion.cpp 와 동일)
VERSION_TABLE = {
    0x59919494: 0x01FB94A8,  # Korean PK 1.0.0
    0x5D834FFA: 0x01FBC618,  # Korean PK 1.0.1
    0x5E3BA719: 0x01FBC5D8,  # Korean PK 1.0.2
}

# 알려진 필드 (docs/specs/OFFSETS.md §4) — diff 출력에 이름을 붙이는 용도
KNOWN_RANGES = [
    (0x0000, 0x0060, "헤더"),
    (0x005E, 0x016D, "인연 성립 비트셋[2165]"),
    (0x016D, 0x09E2, "호감/상성 int8[2165]"),
    (0x09E2, 0x1257, "친밀도 uint8[2165]"),
    (0x1257, 0x1284, "후미 스칼라 필드"),
    (0x1284, 0x12BC, "친애/의형제/원수 슬롯"),
]


def find_pid():
    for pid in os.listdir("/proc"):
        if not pid.isdigit():
            continue
        try:
            with open(f"/proc/{pid}/cmdline", "rb") as f:
                if PROCESS_NAME.lower().encode() in f.read().lower():
                    return int(pid)
        except OSError:
            continue
    return None


def module_base(pid):
    with open(f"/proc/{pid}/maps") as f:
        for line in f:
            if PROCESS_NAME.lower() in line.lower():
                return int(line.split("-", 1)[0], 16)
    return None


def root_base(pid, mem):
    base = module_base(pid)
    if base is None:
        raise SystemExit("게임 모듈을 찾을 수 없습니다")
    mem.seek(base + 0x3C)
    pe = struct.unpack("<I", mem.read(4))[0]
    mem.seek(base + pe + 8)
    stamp = struct.unpack("<I", mem.read(4))[0]
    if stamp not in VERSION_TABLE:
        raise SystemExit(f"지원하지 않는 게임 버전 (TimeDateStamp {stamp:#x})")
    return base + VERSION_TABLE[stamp]


def region_name(off):
    for lo, hi, name in KNOWN_RANGES:
        if lo <= off < hi:
            return name
    return "?"


def cmd_dump(args):
    pid = find_pid()
    if pid is None:
        raise SystemExit("게임 프로세스가 실행 중이 아닙니다")
    with open(f"/proc/{pid}/mem", "rb", 0) as mem:
        rb = root_base(pid, mem)
        mem.seek(rb + DATE_OFFSET)
        y, m, _, d = struct.unpack("<HHHH", mem.read(8))
        mem.seek(rb + OFFICER_POOL_OFFSET)
        data = mem.read(OFFICER_STRIDE * OFFICER_COUNT)
    with open(args.out, "wb") as f:
        f.write(data)
    print(f"저장: {args.out} ({len(data):,} B), 게임 일자 {y}년 {m + 1}월 {d}일, RootBase {rb:#x}")


def cmd_diff(args):
    a = open(args.before, "rb").read()
    b = open(args.after, "rb").read()
    ids = [int(x) for x in args.ids.split(",")] if args.ids else range(OFFICER_COUNT)
    per_offset = defaultdict(list)  # off -> [(id, before, after)]
    for i in ids:
        ra = a[i * OFFICER_STRIDE:(i + 1) * OFFICER_STRIDE]
        rb_ = b[i * OFFICER_STRIDE:(i + 1) * OFFICER_STRIDE]
        for off in range(OFFICER_STRIDE):
            if ra[off] != rb_[off]:
                per_offset[off].append((i, ra[off], rb_[off]))
    rows = [(off, ch) for off, ch in per_offset.items() if len(ch) >= args.min_officers]
    rows.sort()
    print(f"바뀐 오프셋 {len(rows)}개 (무장 {args.min_officers}명 이상 변화 기준)")
    for off, ch in rows:
        deltas = Counter((after - before) & 0xFF for _, before, after in ch)
        sample = ", ".join(f"#{i}:{bv}->{av}" for i, bv, av in ch[:4])
        print(f"  +{off:#06x} [{region_name(off)}] 무장 {len(ch):4d}명  증감(mod 256) {deltas.most_common(3)}  예: {sample}")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    d = sub.add_parser("dump")
    d.add_argument("out")
    df = sub.add_parser("diff")
    df.add_argument("before")
    df.add_argument("after")
    df.add_argument("--ids", default="")
    df.add_argument("--min-officers", type=int, default=1)
    args = p.parse_args()
    {"dump": cmd_dump, "diff": cmd_diff}[args.cmd](args)


if __name__ == "__main__":
    main()
