"""Assemble every maintained far-heap/gate configuration from the same source."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOLS = Path(os.environ.get('XTOOLS_BIN', ROOT/'bin/x/bin'))

class CooperativeMemory(unittest.TestCase):
    def test_gate_ownership_and_platform_configurations(self):
        (ROOT/'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT/'build', prefix='cooperative-memory-') as directory:
            obj = Path(directory)/'memory.rel'
            for cpc, external in ((0, 0), (0, 1), (1, 1), (1, 0)):
                with self.subTest(cpc=cpc, external=external):
                    result = subprocess.run([str(TOOLS/'xas'), '--mode=sdcc',
                        f'-DYOS_CPC_AMSDOS={cpc}', f'-DYOS_EXTERNAL_FAR_GATE={external}',
                        '-I'+str(ROOT/'y/platforms/partner'),
                        '-I'+str(ROOT/'y/src/z80/cooperative/list'),
                        '-o', str(obj), str(ROOT/'y/src/z80/cooperative/farheap/memory.s')],
                        capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    symbols = dict(re.findall(r'^S (\S+) (Def|Ref)', obj.read_text(), re.M))
                    for name in ('__far_getb', '__far_putb'):
                        self.assertEqual(symbols[name], 'Ref' if cpc or external else 'Def')
                    self.assertEqual(symbols['_malloc'], 'Def')
                    self.assertEqual(symbols['_free'], 'Def')

    def test_standalone_and_resident_partner_gates(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'build', prefix='partner-gates-') as directory:
            for resident in (0, 1):
                for name in ('yos/bank.s', 'floppy/floppy.s'):
                    with self.subTest(resident=resident, source=name):
                        obj = Path(directory)/'gate.rel'
                        result = subprocess.run([str(TOOLS/'xas'), '--mode=sdcc',
                            f'-DYOS_RESIDENT_DESKTOP={resident}',
                            '-I'+str(ROOT/'y/platforms/partner'),
                            '-I'+str(ROOT/'y/platforms/partner/floppy'),
                            '-o', str(obj), str(ROOT/'y/platforms/partner'/name)],
                            capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        symbols = dict(re.findall(r'^S (\S+) (Def|Ref)', obj.read_text(), re.M))
                        if name == 'yos/bank.s':
                            self.assertEqual(symbols['__partner_bank_select'], 'Def')
                            self.assertEqual(symbols.get('__partner_bank_gateway_end') == 'Def', not resident)
                        else:
                            self.assertEqual(symbols.get('__partner_bank_select') == 'Ref', bool(resident))
                            fixed = re.search(r'^A _FIXED_CODE size ([0-9A-Fa-f]+)', obj.read_text(), re.M)
                            self.assertEqual(bool(fixed and int(fixed[1], 16)), bool(resident))

if __name__ == '__main__':
    unittest.main(verbosity=2)
