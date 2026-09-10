import pathlib
import re
import subprocess
import sys

compiler, source, directory = sys.argv[1:]
directory = pathlib.Path(directory)
directory.mkdir(parents=True, exist_ok=True)
for abi in (0, 1):
    for profile in ('O0', 'Os', 'Of'):
        stem = directory / f'{profile}-abi{abi}'
        result = subprocess.run(
            [compiler, '-' + profile, '--sdcccall', str(abi), '--dump-ir',
             '-S', source, '-o', str(stem.with_suffix('.s'))],
            text=True, capture_output=True, check=True)
        stem.with_suffix('.ir').write_text(result.stdout)
        functions = dict(re.findall(
            r'; === function (\w+) ===\n(.*?)(?=; === function|$)',
            result.stdout, re.S))
        for name in ('large_ordinary', 'repeated_blocks', 'aligned_large',
                     'nested_partial', 'bitfield_union', 'pointer_to_volatile',
                     'initializer_calls', 'complete_calls', 'designated_duplicates'):
            assert 'block_fill(' in functions[name], (name, functions[name])
        for name in ('volatile_member', 'atomic_member', 'volatile_pointer',
                     'complete_literals'):
            assert 'block_fill(' not in functions[name], (name, functions[name])
        assert re.search(r'block_fill\(t\d+, #0, #304\)', functions['large_ordinary'])
        assert re.search(r'block_fill\(t\d+, #0, #257\)', functions['aligned_large'])
        calls = functions['complete_calls']
        assert calls.index('block_fill(') < calls.index('call init_first') < calls.index('call init_second'), calls
        literals = functions['complete_literals']
        # These objects have complete literal byte coverage: no preliminary
        # zero-byte stores are needed. The values themselves contain no zero.
        assert not re.search(r'^  \*t\d+ = #0$', literals, re.M), literals
print('aggregate zero-fill and literal coverage: 6 profile/ABI IR checks passed')
