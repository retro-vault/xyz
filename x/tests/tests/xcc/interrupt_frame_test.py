#!/usr/bin/env python3
"""Interrupt-style stack use after each generated argument spill preserves arguments."""
from pathlib import Path
import re
import subprocess
import sys
root=Path(__file__).resolve().parents[4]
build=root/'build/tests/xcc-interrupt-frame';build.mkdir(parents=True,exist_ok=True)
tools=Path(sys.argv[1]).resolve().parent if len(sys.argv)>1 else root/'bin/x/bin'
source=build/'probe.c'
source.write_text('''
volatile int a=17,b=25;
int sum(int x,int y){volatile int pad[4];pad[0]=x;pad[1]=y;return pad[0]+pad[1];}
int pointers(const int *x,const int *y){volatile int pad[3];pad[0]=*x;pad[1]=*y;return pad[0]+pad[1];}
int bytes(unsigned char x,unsigned char y){volatile int pad[3];pad[0]=x;pad[1]=y;return pad[0]+pad[1];}
int main(void){int x=a,y=b;return sum(x,y)!=42 || pointers(&x,&y)!=42 || bytes(17,25)!=42;}
''')
for opt in ('-O0','-O1','-O2','-O3','-Os','-Of'):
 asm=build/(opt[1:]+'.s')
 subprocess.run([tools/'xcc','--platform=emu',opt,'-S',source,'-o',asm],check=True)
 text=asm.read_text();lines=text.splitlines(True);head=False;count=0;output=[]
 for line in lines:
  if '; sdcccall(1) prologue:' in line:head=True
  if re.search(r'; (?:receive \(|xopt ordinary ix span:|epilogue:)',line):head=False
  output.append(line)
  if head and re.search(r'\bld\s+-\d+\(ix\),',line):
   output.append('\tcall\t_irq_probe\n');count+=1
 # CALL plus a register-preserving IRQ-sized stack footprint. The former
 # compiler wrote into precisely this unreserved region of the stack.
 output.extend(['\n_irq_probe:\n']+['\tpush\taf\n']*24+['\tpop\taf\n']*24+['\tret\n'])
 asm.write_text(''.join(output))
 binary=asm.with_suffix('.bin')
 subprocess.run([tools/'xcc','--platform=emu','--oformat=binary',asm,'-o',binary],check=True)
 subprocess.run([tools/'xemu','--run','--quiet','--emu-exit-status','--max-steps','2000000','--load-bin',binary],check=True)
 print('PASS',opt,'interrupt stack after',count,'argument spills',flush=True)
