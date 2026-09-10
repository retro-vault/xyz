import pathlib
import subprocess
import sys
compiler, source, work = sys.argv[1:]
work = pathlib.Path(work)
work.mkdir(parents=True, exist_ok=True)
for abi in (0, 1):
    for profile in ('O0', 'Os', 'Of'):
        result = subprocess.run([compiler, '-' + profile, '--sdcccall', str(abi),
                                 '-S', source, '-o', str(work / f'{profile}-{abi}.s')],
                                text=True, capture_output=True)
        (work / f'{profile}-{abi}.log').write_text(result.stdout + result.stderr)
        assert result.returncode == 0, result.stderr
print('qualifier type assertions: 6 profile/ABI checks passed')
