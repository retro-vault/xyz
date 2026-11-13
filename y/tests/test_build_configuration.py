"""Changing YOS build policy must invalidate assembly objects in-place."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
class Configuration(unittest.TestCase):
    def test_flags_invalidate_existing_objects(self):
        with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='yos-config-') as temporary:
            work=Path(temporary);assembler=work/'assembler'
            assembler.write_text('#!/usr/bin/env python3\nimport pathlib,sys\npathlib.Path(sys.argv[sys.argv.index("-o")+1]).write_text(" ".join(sys.argv[1:]))\n')
            assembler.chmod(0o755)
            target=work/'main.rel'
            def build(boot,unified,banks):
                subprocess.run(['make','-s','-C',str(ROOT/'y/src/z80'),
                    'BUILD_DIR='+str(work),'XAS='+str(assembler),
                    'YOS_BOOT_ALTO='+str(boot),'YOS_UNIFIED_48K='+str(unified),
                    'YOS_BANK_COUNT='+str(banks),str(target)],check=True)
                return target.read_text(),target.stat().st_mtime_ns
            old,_=build(0,0,126)
            for boot,unified,banks in ((1,0,126),(1,1,126),(1,1,8),(0,0,126)):
                new,mtime=build(boot,unified,banks)
                self.assertNotEqual(new,old)
                self.assertIn('-DYOS_BOOT_ALTO='+str(boot),new)
                self.assertEqual(build(boot,unified,banks),(new,mtime))
                old=new

if __name__=='__main__':unittest.main(verbosity=2)
