#!/usr/bin/env python3
"""Independent C23 type oracle: explicit rank order and mathematical ranges."""
import itertools
import pathlib
import subprocess
import sys

program = pathlib.Path(sys.argv[1]).resolve()
standard = {
    'bool': (1, False), 'char': (8, False), 'schar': (8, True), 'uchar': (8, False),
    'short': (16, True), 'ushort': (16, False), 'int': (16, True), 'uint': (16, False),
    'long': (32, True), 'ulong': (32, False), 'llong': (64, True), 'ullong': (64, False),
    'enum': (16, True), 'char8': (8, False),
}
# Build rank order independently by inserting standard types after equal-width
# bit-precise types, with short before int on this 16-bit target.
rank = {}
for width in range(1, 65):
    group = len(set(rank.values()))
    rank['s' + str(width)] = rank['u' + str(width)] = group
    if width == 8:
        group += 1
        for name in ('char', 'schar', 'uchar', 'char8'): rank[name] = group
    if width == 16:
        group += 1
        rank['short'] = rank['ushort'] = group
        group += 1
        rank['int'] = rank['uint'] = rank['enum'] = group
    if width == 32:
        rank['long'] = rank['ulong'] = group + 1
    if width == 64:
        rank['llong'] = rank['ullong'] = group + 1
names = list(standard) + ['s' + str(n) for n in range(2, 65)] + ['u' + str(n) for n in range(1, 65)]
pairs = list(itertools.product(names, repeat=2))
checks = 0
for char_signed in (False, True):
    def details(name):
        if name == 'char': return 8, char_signed
        return standard[name] if name in standard else (int(name[1:]), name[0] == 's')
    def limits(name):
        width, signed = details(name)
        return (-(1 << (width-1)), (1 << (width-1))-1) if signed else (0, (1 << width)-1)
    def promote(name):
        if name in ('bool','char','schar','uchar','short','ushort','enum','char8'):
            lo, hi = limits(name)
            return 'int' if -32768 <= lo and hi <= 32767 else 'uint'
        return name
    def unsigned(name):
        return {'int':'uint','long':'ulong','llong':'ullong'}.get(name, 'u'+name[1:])
    def common(a,b):
        a,b=promote(a),promote(b)
        if details(a)[1] == details(b)[1]: return max((a,b),key=lambda n:rank[n])
        signed = a if details(a)[1] else b
        unsign = b if details(a)[1] else a
        if rank[unsign] >= rank[signed]: return unsign
        if limits(signed)[0] <= limits(unsign)[0] and limits(signed)[1] >= limits(unsign)[1]: return signed
        return unsigned(signed)
    result = subprocess.run([str(program), 'signed' if char_signed else 'unsigned'],
                            input=''.join(f'{a} {b}\n' for a,b in pairs), text=True,
                            capture_output=True, check=True)
    rows=result.stdout.splitlines()
    assert len(rows)==len(pairs)
    for (a,b),line in zip(pairs,rows):
        expected=(promote(a),promote(b),common(a,b))
        assert tuple(line.split())==expected,(char_signed,a,b,expected,line)
        checks += 3
print(checks,'independent promotion/common-type checks passed')
