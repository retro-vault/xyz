#!/usr/bin/env python3
# ihx2bin.py — convert Intel HEX to flat binary, trimmed to last byte.
import sys, pathlib

def convert(ihx_path, bin_path):
    buf = bytearray(0x10000)
    last = 0
    with open(ihx_path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith(':'):
                continue
            count = int(line[1:3], 16)
            addr  = int(line[3:7], 16)
            typ   = int(line[7:9], 16)
            data  = bytes.fromhex(line[9:9+count*2])
            if typ == 0:
                for i, b in enumerate(data):
                    buf[addr+i] = b
                    if addr+i > last:
                        last = addr+i
    pathlib.Path(bin_path).write_bytes(bytes(buf[:last+1]))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f'usage: {sys.argv[0]} input.ihx output.bin')
        sys.exit(1)
    convert(sys.argv[1], sys.argv[2])
