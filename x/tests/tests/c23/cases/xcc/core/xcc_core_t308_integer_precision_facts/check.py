import hashlib
import json
import pathlib
import subprocess
import sys

compiler = pathlib.Path(sys.argv[1]).resolve()
work = pathlib.Path(sys.argv[2])
work.mkdir(parents=True, exist_ok=True)
emulator = compiler.parent / 'xemu'
if not emulator.exists():
    # The development compiler facade can point at a private executable.
    emulator = pathlib.Path.cwd() / 'bin/x/bin/xemu'
profiles = ('O0', 'O1', 'O2', 'O3', 'Os', 'Of')
rows = []
checks = 0
for group, widths in enumerate(((1, 2, 3, 7), (8, 9, 10, 11),
                               (12, 13, 14, 15), (16, 17, 23, 31))):
    functions = []
    assertions = []
    for bits in widths:
        outside = 1 << bits
        mask = outside - 1
        functions.append(f'unsigned long zero_{bits}(unsigned long x) {{ return (unsigned long)(unsigned _BitInt({bits}))(x | {outside}UL) & {outside}UL; }}')
        functions.append(f'unsigned long value_{bits}(unsigned long x) {{ return (unsigned long)(unsigned _BitInt({bits}))(x | {outside}UL); }}')
        assertions.extend([f'if (zero_{bits}(x) != 0UL) return 1;',
                           f'if (value_{bits}(x) != (x & {mask}UL)) return 2;'])
        if bits > 1:
            sign = 1 << (bits - 1)
            functions.append(f'unsigned long negative_{bits}(unsigned long x) {{ return (unsigned long)(_BitInt({bits}))(x | {sign}UL) & {outside}UL; }}')
            functions.append(f'unsigned long positive_{bits}(unsigned long x) {{ return (unsigned long)(_BitInt({bits}))(x & {sign - 1}UL) & {outside}UL; }}')
            assertions.extend([f'if (negative_{bits}(x) != {outside}UL) return 3;',
                               f'if (positive_{bits}(x) != 0UL) return 4;'])
        if 9 <= bits <= 15:
            functions.extend([
                f'unsigned add_{bits}(unsigned x) {{ return (unsigned)(unsigned _BitInt({bits}))(signed char)x + 1u; }}',
                f'unsigned eq_const_{bits}(unsigned x) {{ return (unsigned _BitInt({bits}))(signed char)x == (unsigned)-1; }}',
                f'unsigned eq_{bits}(unsigned x) {{ return (unsigned _BitInt({bits}))(signed char)x == (int)(signed char)x; }}',
                f'unsigned lt_{bits}(unsigned x) {{ return (unsigned _BitInt({bits}))(signed char)x < (int)(signed char)x; }}',
                f'unsigned gt_{bits}(unsigned x) {{ return (unsigned _BitInt({bits}))(signed char)x > (int)(signed char)x; }}',
                f'unsigned xor_{bits}(unsigned x) {{ return (unsigned)(unsigned _BitInt({bits}))(signed char)x ^ 37u; }}',
            ])
            assertions.extend([
                f'if (add_{bits}(i) != ((i < 128u ? i : i + {outside - 256}u) + 1u)) return 5;',
                f'if (eq_const_{bits}(i) != 0u) return 6;',
                f'if (eq_{bits}(i) != (i < 128u)) return 7;',
                f'if (lt_{bits}(i) != 0u) return 8;',
                f'if (gt_{bits}(i) != (i >= 128u)) return 9;',
                f'if (xor_{bits}(i) != ((i < 128u ? i : i + {outside - 256}u) ^ 37u)) return 10;',
            ])
    if group == 0:
        functions.extend([
            'unsigned long count_left(unsigned long x) { return (x << (unsigned _BitInt(4))10) << (unsigned _BitInt(4))10; }',
            'unsigned long count_right(unsigned long x) { return (x >> (unsigned _BitInt(4))10) >> (unsigned _BitInt(4))10; }',
            'unsigned long signed_count_left(unsigned long x) { return (x << (_BitInt(4))6) << (_BitInt(4))6; }',
            'unsigned long signed_count_right(unsigned long x) { return (x >> (_BitInt(4))6) >> (_BitInt(4))6; }',
        ])
        assertions.extend([
            'if (count_left(x) != (x << 20)) return 11;',
            'if (count_right(x) != (x >> 20)) return 12;',
            'if (signed_count_left(x) != (x << 12)) return 13;',
            'if (signed_count_right(x) != (x >> 12)) return 14;',
        ])
    # Word-typed loop inputs prevent compile-time evaluation of test calls.
    code = '\n'.join(functions) + '\nvolatile unsigned dynamic_input;\n'
    code += 'int main(void) { unsigned i; unsigned long x; for (i=0; i<256u; ++i) { dynamic_input=i; x=(unsigned long)dynamic_input * 16843009UL;\n'
    code += '\n'.join(assertions) + '\n} return 0; }\n'
    source = work / f'group{group}.c'
    source.write_text(code)
    for abi in (0, 1):
        for profile in profiles:
            stem = work / f'group{group}-{profile}-abi{abi}'
            command = [str(compiler), '--platform=emu', '--oformat=binary',
                       '-' + profile, '--sdcccall', str(abi), str(source),
                       '-o', str(stem.with_suffix('.bin'))]
            result = subprocess.run(command, text=True, capture_output=True)
            stem.with_suffix('.compile.log').write_text(result.stdout + result.stderr)
            assert result.returncode == 0, (command, result.stderr)
            run_command = [str(emulator), '--run', '--quiet', '--load-bin',
                           str(stem.with_suffix('.bin')), '--origin', '0', '--pc', '0',
                           '--emu-stdio', '--emu-exit-status', '--max-steps', '20000000']
            result = subprocess.run(run_command, text=True, capture_output=True)
            stem.with_suffix('.run.log').write_text(result.stdout + result.stderr)
            rows.append(dict(group=group, profile=profile, abi=abi,
                             command=command, run_command=run_command,
                             exit=result.returncode, checks=len(assertions) * 256,
                             source_sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
            (work / 'validation.json').write_text(json.dumps(rows, indent=2) + '\n')
            assert result.returncode == 0, (stem, result.returncode, result.stdout, result.stderr)
            checks += len(assertions) * 256
            print(stem.name, 'PASS', flush=True)
# Ordinary byte widening still lowers without a separate widened temporary.
positive = work / 'ordinary.c'
positive.write_text('unsigned ordinary(unsigned char x) { return (unsigned)x + 1u; }\n')
for profile in ('Os', 'Of'):
    result = subprocess.run([str(compiler), '-' + profile, '--dump-ir', '-S',
                             str(positive), '-o', str(work / f'ordinary-{profile}.s')],
                            text=True, capture_output=True, check=True)
    (work / f'ordinary-{profile}.ir').write_text(result.stdout)
    assert '(unsigned int)' not in result.stdout, result.stdout
print(f'integer precision: {checks} value checks across {len(rows)} profile/ABI images; ordinary widening retained')
