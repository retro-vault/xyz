from pathlib import Path
import os
import re
import subprocess
import sys

compiler,source,directory=sys.argv[1:]
prefix=Path(compiler).resolve().parent.parent
work=Path(directory).resolve();work.mkdir(parents=True,exist_ok=True)
repo=next(p for p in Path(__file__).resolve().parents if (p/'x/lib/xz80/include/xz80/xz80.h').exists())
probe=work/'probe'
subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O2','-I'+str(repo/'x/lib/xz80/include'),str(repo/'x/tests/tests/xopt/shift_xor_conversion_probe.cpp'),str(prefix/'lib/libxz80.a'),'-o',str(probe)],check=True)
checks=0
for abi in (0,1):
    for profile in ('O0','O1','O2','O3','Of','Os'):
        stem=work/f'abi{abi}-{profile}'
        subprocess.run([compiler,'-S','-'+profile,'--sdcccall',str(abi),source,'-o',str(stem.with_suffix('.s'))],check=True)
        subprocess.run([str(prefix/'bin/xas'),str(stem.with_suffix('.s')),'-o',str(stem.with_suffix('.rel'))],check=True)
        subprocess.run([str(prefix/'bin/xld'),'-nostdlib','--no-default-runtime','--oformat=binary','--section-start=_CODE=0x100','--section-start=_BSS=0xc000','--binary-range=0-0xffff','-e','_f0','-Map='+str(stem.with_suffix('.map')),str(stem.with_suffix('.rel')),str(prefix/'z80/lib/libruntime.a'),'-o',str(stem.with_suffix('.bin'))],check=True)
        symbols={m[2]:int(m[1],16) for m in re.finditer(r'^([0-9A-Fa-f]{8}) (\S+)',stem.with_suffix('.map').read_text(),re.M)}
        stem.with_suffix('.cases').write_text(''.join(f'{symbols[f"_f{kind}"]} {kind}\n' for kind in range(12)))
        result=subprocess.run([str(probe),str(stem.with_suffix('.bin')),str(stem.with_suffix('.cases')),str(abi)],capture_output=True,text=True)
        (work/f'abi{abi}-{profile}.log').write_text(result.stdout+result.stderr)
        assert result.returncode==0,result.stdout+result.stderr
        checks+=int(result.stdout.split()[0])
print(checks,'independent shift/XOR conversion values passed')
