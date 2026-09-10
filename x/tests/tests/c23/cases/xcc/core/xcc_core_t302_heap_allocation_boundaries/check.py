from pathlib import Path
import os
import subprocess
import sys

compiler, directory = sys.argv[1:]
prefix=Path(compiler).resolve().parent.parent
work=Path(directory).resolve();work.mkdir(parents=True,exist_ok=True)
repo=next(p for p in Path(__file__).resolve().parents if (p/'x/lib/xz80/include/xz80/xz80.h').exists())
def run(command):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True)
    if result.returncode:
        raise AssertionError(result.stdout+result.stderr)
    return result.stdout
objects=[]
for name in ('calloc','heap_core','realloc','libc_align_size','heap_init_arena'):
    obj=work/(name+'.rel')
    run([prefix/'bin/xas',repo/f'x/libc/src/stdlib/{name}.s','-o',obj])
    objects.append(obj)
probe=work/'heap-calloc-probe'
run([os.environ.get('CXX','c++'),'-std=c++20','-O2','-I'+str(repo/'x/lib/xz80/include'),repo/'x/tests/tests/libc/heap_calloc_probe.cpp',prefix/'lib/libxz80.a','-o',probe])
for mode in ('isolated','heap','heap-odd'):
    source=work/(mode+'-stub.s');obj=work/(mode+'-stub.rel')
    source.write_text('        .area _CODE\n'+(
        '        .globl _malloc\n_malloc::\n        ret\n' if mode=='isolated' else
        '        .globl _heap_region\n_heap_region::\n        ld hl,#'+('0x4001' if mode=='heap-odd' else '0x4000')+'\n        ld de,#0xc000\n        ret\n        .globl _aligned_alloc\n        .dw _aligned_alloc\n'))
    run([prefix/'bin/xas',source,'-o',obj])
    image=work/(mode+'.bin');mapping=work/(mode+'.map')
    inputs=([objects[0]] if mode=='isolated' else objects)+[obj]
    libs=[] if mode=='isolated' else [prefix/'z80/lib/libc.a']
    run([prefix/'bin/xld','-nostdlib','--no-default-runtime','--oformat=binary','--section-start=_CODE=0x100','--section-start=_DATA=0x3000','--section-start=_BSS=0x3800','--binary-range=0-0xffff','-e','_calloc','-Map='+str(mapping),*inputs,*libs,prefix/'z80/lib/libruntime.a','-o',image])
    result=run([probe,image,mapping,mode]);(work/(mode+'.log')).write_text(result)
    print(result,end='')
