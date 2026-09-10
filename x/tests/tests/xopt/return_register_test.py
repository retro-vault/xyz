#!/usr/bin/env python3
"""Preserve every declared return register, including wide and unknown ABIs."""
import os
import contextlib
import pathlib
import re
import subprocess
import sys
import tempfile


def main():
    compiler = pathlib.Path(sys.argv[1]).resolve()
    prefix = compiler.parent.parent
    optimizer = prefix / 'bin/xopt'
    repository = next(p for p in pathlib.Path(__file__).resolve().parents
                      if (p / 'x/lib/xz80/include/xz80/xz80.h').exists())
    bodies = [
        # The high word may be live even when its temporary stack stores die.
        'ld l,a\nld h,#0\nld -4(ix),l\nld -3(ix),h\n'
        'ld hl,#0\nld -2(ix),l\nld -1(ix),h\nld e,-4(ix)\nld d,-3(ix)\n',
        'ld hl,#4660\nld d,h\nld e,l\n',
        'ld l,-4(ix)\nld h,-3(ix)\nld d,h\nld e,l\n',
        'ld d,h\nld e,l\n',
        'ld de,#43981\nld hl,#4660\nex de,hl\n',
        'ld b,h\nld c,l\nld d,b\nld e,c\n',
        'ld de,#0\nld -4(ix),e\nld -3(ix),d\n',
        'ld bc,#0\nld -4(ix),c\nld -3(ix),b\n',
        # Both return branches must retain their own high word.
        'or a\njr z,_zero_CASE\nld hl,#4660\nld -4(ix),l\nld -3(ix),h\n'
        'ld de,#22136\njr _end_CASE\n_zero_CASE:\nld hl,#0\n'
        'ld -4(ix),l\nld -3(ix),h\nld de,#1\n_end_CASE:\n',
    ]
    # Unknown/malformed annotations preserve all registers. Masks also cover
    # byte, word, long, 64-bit, and void results without guessing from ABI1.
    contracts = [(None, 2047), ('oops', 2047), ('2048', 2047)] + [
        (str(mask), mask) for mask in (0, 1, 24, 64, 96, 120, 2040, 2047)]
    with (contextlib.nullcontext(os.environ['XOPT_TEST_WORK'])
          if 'XOPT_TEST_WORK' in os.environ else tempfile.TemporaryDirectory()) as temporary:
        work = pathlib.Path(temporary)
        probe = work / 'probe'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-O2',
                        '-I' + str(repository / 'x/lib/xz80/include'),
                        str(pathlib.Path(__file__).with_name('return_register_probe.cpp')),
                        str(prefix / 'lib/libxz80.a'), '-o', str(probe)], check=True)
        source = work / 'original.s'
        functions, masks = [], []
        for body in bodies:
            for annotation, mask in contracts:
                case = len(masks)
                metadata = '' if annotation is None else ', return_regs=' + annotation
                # Each visit initializes the complete frame before using it.
                functions.append(
                    f'\t.area _CODE\n\t.globl _case{case}\n_case{case}:\n'
                    f'; sdcccall(1) prologue: case{case} (locals=0, temp_frame=4, stack_params=0{metadata})\n'
                    '\tpush ix\n\tld ix,#0\n\tadd ix,sp\n'
                    '\tld hl,#-4\n\tadd hl,sp\n\tld sp,hl\n'
                    '\tld -4(ix),a\n\tld -3(ix),e\n\tld hl,#4660\n' +
                    ''.join('\t' + line + '\n' for line in body.replace('CASE', str(case)).splitlines()) +
                    '\tld sp,ix\n\tpop ix\n\tret\n')
                masks.append(mask)
        # An unannotated public function following a narrow annotated one
        # must not inherit that earlier contract. Cover both declaration
        # syntaxes, including a declaration distant from its function body.
        for spelling in ('global', 'globl', 'colon'):
            case = len(masks)
            functions.append(
                f'\t.area _CODE\n_prior{case}:\n'
                f'; sdcccall(1) prologue: prior{case} (return_regs=24)\n'
                '\tld de,#7\n\tret\n')
            label = f'_case{case}:' + (':' if spelling == 'colon' else '')
            if spelling != 'colon':
                functions.insert(0, f'\t.{spelling} _case{case}, _prior{case}\n')
            functions.append(label + '\n\tld hl,#4660\n\tld d,h\n\tld e,l\n\tret\n')
            masks.append(2047)
        original = ''.join(functions)
        source.write_text(original)
        for profile in ('-O2', '-Os', '-Of', '-O3'):
            output = work / 'optimized.s'
            subprocess.run([str(optimizer), profile, str(source), '-o', str(output)], check=True)
            optimized = output.read_text()
            # The entire TU has enough repetitions to exercise outlining too.
            if profile == '-Os':
                assert '__xopt_outline_' in optimized
            combined = work / 'combined.s'
            combined.write_text(original.replace('_case', '_original').replace('_zero_', '_origzero_').replace('_end_', '_origend_') + optimized)
            obj, binary, mapping = [work / name for name in ('out.rel', 'out.bin', 'out.map')]
            subprocess.run([str(prefix / 'bin/xas'), str(combined), '-o', str(obj)], check=True)
            subprocess.run([str(prefix / 'bin/xld'), '-nostdlib', '--no-default-runtime',
                            '--oformat=binary', '--section-start=_CODE=0x100',
                            '--binary-range=0-0xffff', '-e', '_case0',
                            '-Map=' + str(mapping), str(obj), '-o', str(binary)], check=True)
            symbols = {m[2]: int(m[1], 16) for m in re.finditer(
                r'^([0-9A-Fa-f]{8}) (\S+)', mapping.read_text(), re.MULTILINE)}
            cases = work / 'cases.txt'
            cases.write_text(''.join(f'{symbols[f"_original{i}"]} {symbols[f"_case{i}"]} {mask}\n'
                                     for i, mask in enumerate(masks)))
            subprocess.run([str(probe), str(binary), str(cases)], check=True)


if __name__ == '__main__':
    main()
