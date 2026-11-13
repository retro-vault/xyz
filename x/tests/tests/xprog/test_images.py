#!/usr/bin/env python3
"""Production xcc/xprog contract, including relocation and invalid input."""
import os
from pathlib import Path
import struct
import subprocess
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[4]
BIN = Path(os.environ.get('XTOOLS_BIN', ROOT / 'bin/x/bin'))
WORK = ROOT / 'build/tests/yos-images'
WORK.mkdir(parents=True, exist_ok=True)


def run(tool, *args):
    return subprocess.run([str(BIN/tool), *map(str, args)], cwd=WORK,
                          env={**os.environ, 'TMPDIR': str(WORK)},
                          text=True, capture_output=True)


def word(data, offset):
    return struct.unpack_from('<H', data, offset)[0]


class Images(unittest.TestCase):
    def setUp(self):
        (WORK/'tiny.xl').write_bytes(bytes([
            88, 76, 2, 0, 2, 0, 8, 0, 1, 0, 0, 0,
            0, 0, 201, 205, 0, 0, 201, 201, 4, 0, 2, 0]))
        (WORK/'tiny.map').write_text(
            'Symbols:\n00000002 _entry ; tiny.rel\n00000006 _last\n')
        (WORK/'app.c').write_text('int main(void) { return 0; }\n')
        (WORK/'svc.c').write_text(
            'int value = 42; int zero; int *address(void) { return &value; }\n'
            'int init(void) { value += zero; return zero; }\n')

    def success(self, tool, *args):
        r = run(tool, *args)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

    def image(self, name, kind):
        data = (WORK/name).read_bytes()
        self.assertEqual(data[:6], b'XPRG\x01' + bytes([kind]))
        payload = data[word(data, 10):]
        self.assertEqual(payload[:3], b'XL\x02')
        self.assertEqual(zlib.crc32(payload), struct.unpack_from('<I', data, 16)[0])
        self.assertEqual(data[7] & 1, 0)  # relocatable, no preferred address
        self.assertEqual(word(data, 24), 0)
        self.success('xprog', '--inspect', name)
        return data, payload

    def test_map_export_order_and_repeated_alias(self):
        self.success('xprog', '--service', 'tiny.xl', '--export', '_last',
                     '--export', '_entry', '--export', '_last', '--entry', '_entry',
                     '--map', 'tiny.map', '-o', 'tiny.svc')
        data, _ = self.image('tiny.svc', 2)
        self.assertEqual(data[64:73], bytes([195, 6, 0, 195, 2, 0, 195, 6, 0]))
        self.assertEqual(word(data, 26), 2)

    def test_numeric_exports_stay_compatible(self):
        self.success('xprog', '--service', 'tiny.xl', '--export', '2', '--export', '0x6')
        data, _ = self.image('tiny.svc', 2)
        self.assertEqual(word(data, 34), 2)

    def test_invalid_symbols_and_maps(self):
        cases = [('--export', '_missing', '--map', 'tiny.map'),
                 ('--export', '_entry'),
                 ('--export', '_entry', '--map', 'absent.map'),
                 ('--export', '65536'), ('--export', '-1'),
                 ('--export', '8'), ('--export', '_wide', '--map', 'wide.map'),
                 ('--export', '_entry', '--map', 'duplicate.map')]
        (WORK/'wide.map').write_text('00010000 _wide\n')
        (WORK/'duplicate.map').write_text('00000002 _entry\n00000006 _entry\n')
        for args in cases:
            with self.subTest(args=args):
                self.assertNotEqual(run('xprog', '--service', 'tiny.xl', *args).returncode, 0)

    def test_map_rejected_for_media(self):
        self.assertNotEqual(run('xprog', '--tap', 'tiny.xl', '--map', 'tiny.map').returncode, 0)

    def test_compile_process_defaults_and_metadata(self):
        self.success('xcc', '--prc', 'app.c', '--stack-size=512', '--name', 'portable',
                     '--abi=1', '--min-os', '1', '--id=123', '-o', 'app.prc')
        data, _ = self.image('app.prc', 1)
        self.assertEqual(word(data, 28), 512)
        self.assertEqual(word(data, 30), 1)
        self.assertEqual(data[40:49], b'portable\0')
        self.assertEqual(struct.unpack_from('<I', data, 20)[0], 123)

    def test_compile_service_has_relocations(self):
        self.success('xcc', '--svc', 'svc.c', '--export=address', '--entry=init',
                     '--name=sample', '-o', 'sample.svc', '-Map=sample.map')
        data, payload = self.image('sample.svc', 2)
        self.assertEqual(word(data, 34), 1)
        self.assertEqual(word(data, 30), 1)  # executable by the current YOS loader
        self.assertEqual(word(data, 28), 0)
        self.assertGreater(word(payload, 8), 0)
        self.assertTrue((WORK/'sample.map').is_file())
        self.assertLess(word(data, 65), word(payload, 6))

    def test_one_service_runs_at_three_load_addresses(self):
        self.success('xcc', '--svc', 'svc.c', '--export=address', '--entry=init',
                     '-Map=relocate.map', '-o', 'relocate.svc')
        data, xl = self.image('relocate.svc', 2)
        code_size, count = word(xl, 6), word(xl, 8)
        for base in (0x4000, 0x8000, 0xc000):
            with self.subTest(base=base):
                memory = bytearray(65536)
                memory[base:base+code_size] = xl[12:12+code_size]
                for index in range(count):
                    pos = 12 + code_size + 4 * index
                    offset, width, kind = word(xl, pos), xl[pos+2], xl[pos+3]
                    self.assertEqual(width, 2)
                    self.assertEqual(kind, 0)
                    struct.pack_into('<H', memory, base+offset,
                                     (word(memory, base+offset)+base) & 65535)
                # RST18 service discovery stub: no hosted libc is used here.
                memory[0x18:0x1c] = bytes([0x11, 0, 0, 0xc9])
                entry = base + word(data, 26)
                export = base + word(data, 65)
                # Poison BSS: a zero-filled XL alone must not pass this test.
                symbols = {row.split()[1]: int(row.split()[0], 16)
                           for row in (WORK/'relocate.map').read_text().splitlines()
                           if len(row.split()) >= 2 and len(row.split()[0]) == 8
                           and all(c in '0123456789abcdefABCDEF' for c in row.split()[0])}
                struct.pack_into('<H', memory, base+symbols['_zero'], 0xa5a5)
                code = bytearray([0x31, 0x00, 0xf8])
                failures = []
                def fail_if(opcode):
                    code.extend([opcode, 0])
                    failures.append(len(code)-1)
                code.extend([0xcd, entry & 255, entry >> 8, 0x7a, 0xb3])
                fail_if(0x20)  # initializer must return DE=0
                code.extend([0xcd, export & 255, export >> 8, 0x1a, 0xfe, 42])
                fail_if(0x20)
                code.extend([0x13, 0x1a, 0xb7])
                fail_if(0x20)
                code.extend([0x21, 0, 0, 0x39, 0x11, 0, 0xf8, 0xb7, 0xed, 0x52])
                fail_if(0x20)  # SP must still equal F800
                code.extend([0x21, 0, 0, 0x18, 3])
                failure = len(code)
                code.extend([0x21, 1, 0, 0x22, 0, 0xff,
                             0x3e, 0xa5, 0x32, 2, 0xff, 0x76])
                for offset in failures:
                    code[offset] = failure - offset - 1
                memory[0x100:0x100+len(code)] = code
                (WORK/'relocated.bin').write_bytes(memory)
                self.success('xemu', '--run', '--quiet', '--emu-exit-status',
                             '--pc', '0x100', '--max-steps', '100000',
                             '--load-bin', 'relocated.bin')

    def test_assembly_service_exports(self):
        (WORK/'simple.s').write_text('.area _CODE\n_answer::\n ld de,#42\n ret\n')
        self.success('xcc', '--svc', 'simple.s', '--export', '_answer', '-o', 'simple.svc')
        self.image('simple.svc', 2)

    def test_relocatable_images_reject_fixed_link_layout(self):
        for option in ('-Ttext=0x8000', '--section-start=_CODE=0x8000',
                       '--binary-range=0x8000-0xffff', '-Wl,-b,_CODE=0x8000'):
            with self.subTest(option=option):
                self.assertNotEqual(run('xcc', '--prc', '--stack-size=512',
                                        option, 'app.c', '-o', 'fixed.prc').returncode, 0)

    def test_image_options_require_correct_target_and_link_mode(self):
        for args in [('--prc', '--svc'), ('--prc', '-S'), ('--prc', '-c'),
                     ('--prc', '--platform=emu'), ('--prc', '--oformat=binary'),
                     ('--svc', '--stack-size=512'), ('--prc', '--export=main'),
                     ('--svc',), ('--prc',), ('--stack-size=512',),
                     ('--svc', '--sdcccall=0', '--export=address')]:
            with self.subTest(args=args):
                self.assertNotEqual(run('xcc', *args, 'svc.c').returncode, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
