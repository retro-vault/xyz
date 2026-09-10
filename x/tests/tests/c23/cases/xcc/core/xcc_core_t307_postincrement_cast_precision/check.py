#!/usr/bin/env python3
"""Check post-increment load projection, access order and cursor aliases."""
from pathlib import Path
import os
import subprocess
import sys

compiler, directory = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
root = next(p for p in here.parents if (p / 'x/lib/xz80/include/xz80/xz80.h').exists())
prefix = compiler.resolve().parent.parent
out = directory.resolve()
out.mkdir(parents=True, exist_ok=True)

def run(command):
    result = subprocess.run(list(map(str, command)), text=True, capture_output=True)
    assert result.returncode == 0, (command, result.stdout, result.stderr)
    return result.stdout

probe = out / 'probe'
run([os.environ.get('CXX', 'c++'), '-std=c++20', '-O2',
     '-I' + str(root / 'x/lib/xz80/include'), here / 'probe.cpp',
     prefix / 'lib/libxz80.a', '-o', probe])
checks = projections = 0
for abi in (0, 1):
    for profile in ('O0', 'O1', 'O2', 'O3', 'Of', 'Os'):
        for no_regalloc in (False, True):
            stem = out / f'abi{abi}-{profile}-noreg{int(no_regalloc)}'
            command = [compiler, '-S', '-' + profile, '--sdcccall', str(abi),
                       here / 'input.c', '-o', stem.with_suffix('.s')]
            if no_regalloc:
                command.append('-fno-regalloc')
            run(command)
            run([prefix / 'bin/xas', stem.with_suffix('.s'),
                 '-o', stem.with_suffix('.rel')])
            run([prefix / 'bin/xld', '-nostdlib', '--no-default-runtime',
                 '--oformat=binary', '--section-start=_CODE=0x100',
                 '--section-start=_BSS=0xd000', '--binary-range=0-0xffff',
                 '-e', '_f0_0_1', '-Map=' + str(stem.with_suffix('.map')),
                 stem.with_suffix('.rel'), prefix / 'z80/lib/libruntime.a',
                 '-o', stem.with_suffix('.bin')])
            # These baseline lanes use the ordinary byte projection; other
            # lanes may legitimately select a different register layout.
            require_projection = no_regalloc and profile in ('O0', 'O1')
            result = run([probe, stem.with_suffix('.bin'), stem.with_suffix('.map'),
                          str(abi), str(int(require_projection))])
            (stem.with_suffix('.log')).write_text(result)
            checks += int(result.split()[0])
            projections += int(result.split(';')[1].split()[0])
print(f'{checks} postincrement precision/access cases passed; '
      f'{projections} ordinary byte projections verified')
