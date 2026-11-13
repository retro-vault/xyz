#!/usr/bin/env python3
"""Execute C service-table calls with adjacent byte arguments at every profile."""
from pathlib import Path
import subprocess
import sys
import tempfile

HEADER = r"""
#include <stdint.h>
typedef void (*pixel_t)(void *, int16_t, int16_t, uint8_t, uint8_t, void *);
typedef uint8_t *(*event_t)(uint8_t *, uint8_t);
struct table { void *padding[61]; pixel_t pixel; event_t event; };
extern const struct table api;
extern volatile uint16_t calls, result;
extern uint16_t abi_sp(void);
"""
SOURCE = HEADER + r"""
int main(void) {
    uint16_t n;
    uint8_t event[6]={0};
    for(n=0;n<256;++n) {
        uint16_t before=abi_sp();
        if(api.event(event,(uint8_t)n)!=event)return 6;
        if(abi_sp()!=before)return 7;
        if(event[1]!=(uint8_t)n)return 8;
    }
    for(n=0;n<64;++n) {
        uint16_t before=abi_sp();
        api.pixel((void *)0,1,2,1,0,(void *)0);
        if(abi_sp()!=before)return 1;
        if(result!=4)return 2;
        before=abi_sp();
        api.pixel((void *)3,5,7,11,13,(void *)17);
        if(abi_sp()!=before)return 3;
        if(result!=56)return 4;
    }
    return calls==128 ? 0 : 5;
}
"""
CALLEE = HEADER + r"""
volatile uint16_t calls, result;
static void pixel(void *g, int16_t x, int16_t y, uint8_t c, uint8_t m, void *clip) {
    ++calls;
    result=(uint16_t)g+x+y+c+m+(uint16_t)clip;
}
static uint8_t *event(uint8_t *p, uint8_t state) { p[1]=state; return p; }
const struct table api = {{0},pixel,event};
"""

def main():
    compiler=Path(sys.argv[1]).resolve()
    root=Path(__file__).resolve().parents[4]
    (root/'build').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=root/'build',prefix='indirect-arguments-') as temporary:
        work=Path(temporary); source=work/'probe.c';source.write_text(SOURCE)
        callee=work/'callee.c';callee.write_text(CALLEE)
        helper=work/'stack.s';helper.write_text('.module stack\n.area _CODE\n_abi_sp::\n ld hl,#0\n add hl,sp\n ex de,hl\n ret\n')
        for profile in ('-O0','-O1','-O2','-Os','-Of','-O3'):
            binary=work/(profile[1:]+'.bin')
            subprocess.run([str(compiler),'--platform=emu',profile,'--oformat=binary',str(source),str(callee),str(helper),'-o',str(binary)],check=True)
            subprocess.run([str(compiler.parent/'xemu'),'--run','--quiet','--emu-exit-status','--max-steps','1000000','--load-bin',str(binary)],check=True)
            print('PASS',profile,'128 indirect pixel calls and 256 stack-local event calls',flush=True)

if __name__=='__main__':main()
