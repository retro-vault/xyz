"""Contract tests for downstream YOS source materialization."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('materialize', ROOT/'y/scripts/materialize_platform.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

class Materialization(unittest.TestCase):
    def setUp(self):
        (ROOT/'build').mkdir(exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=ROOT/'build')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.old = m.YROOT
        m.YROOT = self.root/'y'
        self.addCleanup(setattr, m, 'YROOT', self.old)
        (m.YROOT/'platforms/next').mkdir(parents=True)
        (m.YROOT/'kernel.s').write_text('.area _CODE\nentry::\n ret\n')
        self.output = self.root/'stage'
        self.manifest({'kernel.s': {'source':'kernel.s'}})

    def manifest(self, entries):
        (m.YROOT/'platforms/next/sources.json').write_text(json.dumps(entries))

    def test_exact_copy_and_incremental_mtime(self):
        m.materialize('next',self.output)
        file=self.output/'kernel.s'; before=file.stat().st_mtime_ns
        self.assertEqual(file.read_bytes(),(m.YROOT/'kernel.s').read_bytes())
        m.materialize('next',self.output)
        self.assertEqual(file.stat().st_mtime_ns,before)

    def test_only_section_changes(self):
        self.manifest({'kernel.s':{'source':'kernel.s','areas':['_KEXT']}})
        m.materialize('next',self.output)
        self.assertEqual((self.output/'kernel.s').read_text(),'.area _KEXT\nentry::\n ret\n')

    def test_layout_count_mismatch_is_clear(self):
        for areas in ([],['_CODE','_EXTRA']):
            with self.subTest(areas=areas):
                self.manifest({'kernel.s':{'source':'kernel.s','areas':areas}})
                with self.assertRaisesRegex(ValueError,'section layout'):
                    m.materialize('next',self.output)

    def test_reject_injected_instructions(self):
        self.manifest({'kernel.s':{'source':'kernel.s','areas':['_CODE\n jp 0']}})
        with self.assertRaises(ValueError):m.materialize('next',self.output)

    def test_reject_source_and_output_traversal(self):
        for entries in ({'../escape.s':{'source':'kernel.s'}},
                        {'kernel.s':{'source':'../outside.s'}}):
            self.manifest(entries)
            with self.assertRaises(ValueError):m.materialize('next',self.output)

    def test_validate_all_before_writing(self):
        self.manifest({'kernel.s':{'source':'kernel.s'},'bad.s':{'source':'absent.s'}})
        with self.assertRaises(FileNotFoundError):m.materialize('next',self.output)
        self.assertFalse((self.output/'kernel.s').exists())

    def test_remove_stale_owned_sources_keep_foreign_files(self):
        m.materialize('next',self.output)
        (self.output/'caller.txt').write_text('keep')
        self.manifest({})
        m.materialize('next',self.output)
        self.assertFalse((self.output/'kernel.s').exists())
        self.assertEqual((self.output/'caller.txt').read_text(),'keep')

if __name__=='__main__':unittest.main(verbosity=2)
