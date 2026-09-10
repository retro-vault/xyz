from pathlib import Path
import subprocess
import sys

compiler, directory = sys.argv[1:]
work = Path(directory).resolve()
work.mkdir(parents=True, exist_ok=True)
suite = Path(__file__).resolve().parent
repo = next(p for p in suite.parents if (p / 'x/src/xcc/include/frontend/types.h').exists())
driver = work / 'type-driver'
subprocess.run(['g++', '-std=c++17', '-O2', '-DXCC_MODEL_M=1',
    '-I', str(repo / 'x/src/xcc/include'), str(suite / 'type-driver.cpp'),
    str(repo / 'x/src/xcc/src/frontend/types.cpp'), '-o', str(driver)], check=True)
subprocess.run([sys.executable, str(suite / 'type-matrix.py'), str(driver)], check=True)
# Actual compiled type expressions independently check parse-time propagation,
# including standard rank winning ties on this 16-bit-int target.
cases = [
    ('u1','u3','u3'), ('s3','s9','s9'), ('u9','u17','u17'),
    ('s9','s17','s17'), ('s9','u9','u9'), ('s17','u9','s17'),
    ('s9','u17','u17'), ('s16','u16','u16'),
    ('u9','int','int'), ('u16','int','unsigned int'),
    ('s16','unsigned int','unsigned int'), ('s17','unsigned int','s17'),
    ('u17','int','u17'), ('s31','unsigned int','s31'),
    ('u31','long','long'), ('u32','long','unsigned long'),
    ('s32','unsigned long','unsigned long'), ('u32','long long','long long'),
    ('s9','short','int'), ('u9','unsigned short','unsigned int'),
    ('long','int','long'), ('int','short','int'),
]
source = work / 'types-and-constants.c'
lines = []
for width in (1,3,7,9,13,16,17,23,31,32):
    lines.append(f'typedef unsigned _BitInt({width}) u{width};')
    if width > 1: lines.append(f'typedef _BitInt({width}) s{width};')
for left, right, result in cases:
    for a,b in ((left,right),(right,left)):
        lines.append(f'static_assert(_Generic(({a})0 + ({b})0, {result}:1, default:0));')
for width in (1,3,7,9,13,16,17,23,31,32):
    lines.append(f'static_assert(_Generic(+(u{width})1, u{width}:1, default:0));')
    if width > 1: lines.append(f'static_assert(_Generic(-(s{width})1, s{width}:1, default:0));')
lines += [
    'static_assert((unsigned long)((u9)511+(u9)1)==0UL);',
    'static_assert((unsigned long)((u9)0-(u9)1)==511UL);',
    'static_assert((unsigned long)((u9)257*(u9)2)==2UL);',
    'static_assert((unsigned long)((u9)256<<1UL)==0UL);',
    'static_assert((unsigned long)(~(u9)0)==511UL);',
    'static_assert((unsigned long)(-(u9)1)==511UL);',
    'static_assert((long)((s9)-17+(s17)40000L)==39983L);',
    'static_assert((long)((u9)511+1)==512L);',
    'static_assert((unsigned long)((u17)131071UL+(unsigned)1)==0UL);',
    'int main(void) {return 0;}',
]
source.write_text('\n'.join(lines)+'\n')
for profile in ('O0','Os','Of'):
    result = subprocess.run([compiler,'-'+profile,'-S',str(source),'-o',str(work/(profile+'.s'))], capture_output=True,text=True)
    (work/(profile+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode == 0, result.stdout+result.stderr
print('compiled BitInt conversion, expression-type and constant-wrap checks passed')
# The companion constant evaluator oracle owns its own independent AST matrix.
constant_driver = work / 'constant-driver'
subprocess.run(['g++', '-std=c++17', '-O2', '-DXCC_MODEL_M=1',
    '-I', str(repo / 'x/src/xcc/include'), str(suite / 'constant-driver.cpp'),
    str(repo / 'x/src/xcc/src/frontend/types.cpp'),
    str(repo / 'x/src/xcc/src/frontend/const_eval.cpp'), '-o', str(constant_driver)], check=True)
subprocess.run([sys.executable, str(suite / 'constant-matrix.py'), str(constant_driver)], check=True)
