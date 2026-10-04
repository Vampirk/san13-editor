#!/usr/bin/env python3
"""
unpack_s13.py - Romance of the Three Kingdoms 13 PK Archive Unpacker & Data Extractor
Extracts files from KOEI S13 container archives (DATA0.S13 / DATA1.S13, DATA0_2.S13 / DATA1_2.S13).
Supports ZP1 multi-chunk zlib decompression with 128-byte chunk alignment.
"""

import os
import sys
import struct
import zlib
import json
import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
import time

MAGIC_MAP = {
    b'GT1G': '.g1t',
    b'_M1G': '.g1m',
    b'XL\x13\x00': '.xl13',
    b'BGTS': '.bgts',
    b'LHSK': '.lhsk',
    b'DDS ': '.dds',
    b'RIFF': '.wav',
    b'OggS': '.ogg',
    b'\x01\x00\x00\x00': '.tbl',
    b'\x04\x00\x00\x00': '.tbl',
    b'\t\x00\x00\x00': '.tbl',
    b'\x02\x00\x00\x00': '.tbl',
    b'\x03\x00\x00\x00': '.tbl',
}

def detect_extension(data: bytes) -> str:
    if len(data) < 4:
        return '.bin'
    m4 = data[:4]
    return MAGIC_MAP.get(m4, '.dat')

def unpack_single_entry(idx: int, data0_path: str, data1_path: str):
    """
    Reads and decompresses a single entry from S13 archive.
    Returns (idx, status, decompressed_bytes, metadata_dict)
    """
    with open(data0_path, 'rb') as f0:
        f0.seek(idx * 32)
        entry_raw = f0.read(32)
        if len(entry_raw) < 32:
            return (idx, 'eof', b'', {})
        offset, decomp_sz, comp_sz, is_comp = struct.unpack('<4Q', entry_raw)

    meta = {
        'index': idx,
        'offset': offset,
        'decompressed_size': decomp_sz,
        'compressed_size': comp_sz,
        'is_compressed': bool(is_comp),
    }

    if decomp_sz == 0 or comp_sz == 0:
        return (idx, 'empty', b'', meta)

    with open(data1_path, 'rb') as f1:
        f1.seek(offset)
        if not is_comp:
            data = f1.read(decomp_sz)
            meta['format'] = 'raw'
            meta['ext'] = detect_extension(data)
            return (idx, 'ok', data, meta)

        hdr = f1.read(16)
        magic, tot_sz, chunk_sz, num_chunks = struct.unpack('<4sIII', hdr)
        meta['magic'] = magic.decode('ascii', errors='replace')
        meta['num_chunks'] = num_chunks
        meta['chunk_size'] = chunk_sz

        if magic == b'zp1\x02':
            # Stub 4-byte entry
            meta['format'] = 'zp1_stub'
            meta['ext'] = '.bin'
            return (idx, 'ok', b'\x00' * decomp_sz, meta)

        if magic != b'zp1\x00':
            meta['format'] = f'unsupported_{magic!r}'
            return (idx, 'error', b'', meta)

        lens = struct.unpack(f'<{num_chunks}I', f1.read(num_chunks * 4))
        decomp = bytearray()
        rel_pos = 0x800

        for c_idx, c_len in enumerate(lens):
            f1.seek(offset + rel_pos)
            prefix_len = struct.unpack('<I', f1.read(4))[0]
            compressed_chunk = f1.read(prefix_len)
            dec = zlib.decompress(compressed_chunk)
            decomp.extend(dec)
            # Alignment to 128 bytes (0x80)
            rel_pos = (rel_pos + c_len + 0x7F) & ~0x7F

        out_data = bytes(decomp)
        meta['format'] = 'zp1'
        meta['ext'] = detect_extension(out_data)
        return (idx, 'ok', out_data, meta)

def extract_archive(data0_path: str, data1_path: str, out_dir: str, prefix: str = 'file', max_workers: int = 16):
    os.makedirs(out_dir, exist_ok=True)
    size0 = os.path.getsize(data0_path)
    total_entries = size0 // 32

    print(f'[*] Extracting {data0_path} ({total_entries} entries) -> {out_dir}')
    t0 = time.time()

    manifest = []
    success_count = 0
    empty_count = 0
    error_count = 0

    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(unpack_single_entry, i, data0_path, data1_path): i
            for i in range(total_entries)
        }

        for fut in as_completed(futures):
            idx, status, data, meta = fut.result()
            if status == 'ok':
                success_count += 1
                ext = meta.get('ext', '.dat')
                filename = f'{prefix}_{idx:04d}{ext}'
                out_path = os.path.join(out_dir, filename)
                with open(out_path, 'wb') as out_f:
                    out_f.write(data)
                meta['filename'] = filename
                meta['actual_size'] = len(data)
                manifest.append(meta)
            elif status == 'empty':
                empty_count += 1
                meta['status'] = 'empty'
                manifest.append(meta)
            else:
                error_count += 1
                meta['status'] = f'error_{status}'
                manifest.append(meta)

    manifest.sort(key=lambda x: x['index'])
    elapsed = time.time() - t0
    print(f'[+] Extraction complete in {elapsed:.2f}s: {success_count} extracted, {empty_count} empty, {error_count} errors.')
    return manifest

def extract_settlements_master(entry_1655_data: bytes, out_json_path: str, out_csv_path: str):
    """
    Parses master settlement and effect names from Entry 1655.
    """
    sub = entry_1655_data[0x21A00 : 0x2A000]
    raw_tokens = sub.split(b'\x00')
    tokens = []
    for t in raw_tokens:
        try:
            s = t.decode('cp949').strip()
            if len(s) > 0:
                tokens.append(s)
        except Exception:
            pass

    # Settlement names: Token 182 ~ 328 (147 total)
    settlement_names = tokens[182 : 182 + 147]

    # Effects: Token 329 ~ 372 (44 total)
    effects_table = {}
    for i in range(329, min(373, len(tokens))):
        eff_id = i - 329
        effects_table[eff_id] = tokens[i]

    records = []
    for idx, name in enumerate(settlement_names):
        records.append({
            'id': idx,
            'name': name,
            'type': 'base_settlement'
        })

    os.makedirs(os.path.dirname(out_json_path), exist_ok=True)
    with open(out_json_path, 'w', encoding='utf-8') as f:
        json.dump({
            'total_settlements': len(records),
            'settlements': records,
            'effects': effects_table
        }, f, ensure_ascii=False, indent=2)

    with open(out_csv_path, 'w', encoding='utf-8') as f:
        f.write('id,name,type\n')
        for r in records:
            f.write(f"{r['id']},{r['name']},{r['type']}\n")

    print(f'[+] Exported {len(records)} master settlement names to {out_json_path} and {out_csv_path}')
    return records

def main():
    parser = argparse.ArgumentParser(description='San13PK Archive Unpacker')
    parser.add_argument('--game-dir', default='/home/user/.local/share/Steam/steamapps/common/Romance of the Three Kingdoms 13/pk',
                        help='Path to San13PK directory')
    parser.add_argument('--out-dir', default='unpacked_pk',
                        help='Output directory for unpacked assets')
    parser.add_argument('--all', action='store_true',
                        help='Extract all files from DATA0 and DATA0_2')
    parser.add_argument('--extract-settlements', action='store_true',
                        help='Extract master settlements from entry 1655')
    parser.add_argument('--entry', type=int, default=None,
                        help='Extract a single entry number')
    parser.add_argument('--jobs', type=int, default=16,
                        help='Number of parallel worker processes')

    args = parser.parse_args()

    data0_path = os.path.join(args.game_dir, 'DATA0.S13')
    data1_path = os.path.join(args.game_dir, 'DATA1.S13')
    data0_2_path = os.path.join(args.game_dir, 'DATA0_2.S13')
    data1_2_path = os.path.join(args.game_dir, 'DATA1_2.S13')

    if not os.path.exists(data0_path) or not os.path.exists(data1_path):
        print(f'[-] Error: PK archive not found at {args.game_dir}')
        sys.exit(1)

    if args.extract_settlements:
        idx, status, data, meta = unpack_single_entry(1655, data0_path, data1_path)
        if status != 'ok':
            print(f'[-] Failed to unpack entry 1655: {status}')
            sys.exit(1)
        extract_settlements_master(data, 'data/settlements_master.json', 'data/settlements_master.csv')

    if args.entry is not None:
        idx, status, data, meta = unpack_single_entry(args.entry, data0_path, data1_path)
        print(f'[*] Entry {args.entry}: status={status}, meta={meta}')
        if status == 'ok':
            ext = meta.get('ext', '.dat')
            out_file = f'entry_{args.entry:04d}{ext}'
            with open(out_file, 'wb') as f:
                f.write(data)
            print(f'[+] Saved to {out_file} ({len(data)} bytes)')

    if args.all:
        out_base = args.out_dir
        os.makedirs(out_base, exist_ok=True)

        m1 = extract_archive(data0_path, data1_path, os.path.join(out_base, 'data'), prefix='pk', max_workers=args.jobs)
        m2 = []
        if os.path.exists(data0_2_path) and os.path.exists(data1_2_path):
            m2 = extract_archive(data0_2_path, data1_2_path, os.path.join(out_base, 'data2'), prefix='pk2', max_workers=args.jobs)

        manifest_path = os.path.join(out_base, 'manifest.json')
        with open(manifest_path, 'w', encoding='utf-8') as f:
            json.dump({
                'data': m1,
                'data2': m2,
                'extracted_at': time.strftime('%Y-%m-%d %H:%M:%S')
            }, f, indent=2)
        print(f'[+] Master manifest saved to {manifest_path}')

if __name__ == '__main__':
    main()
