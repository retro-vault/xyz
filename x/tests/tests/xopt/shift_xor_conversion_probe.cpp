#include <xz80/xz80.h>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
struct memory:xz80::IMemory{
 std::array<uint8_t,65536> bytes{};
 uint8_t read(uint16_t a)const noexcept override{return bytes[a];}
 void write(uint16_t a,uint8_t v)noexcept override{bytes[a]=v;}
 void word(unsigned a,unsigned v){bytes[a]=v;bytes[a+1]=v>>8;}
};
struct ports:xz80::IPorts{uint8_t in(uint16_t)noexcept override{return 0;}void out(uint16_t,uint8_t)noexcept override{}};
int main(int argc,char**argv){
 if(argc!=4)return 2;memory mem;ports io;xz80::cpu cpu(mem,io);
 std::ifstream binary(argv[1],std::ios::binary);binary.read((char*)mem.bytes.data(),65536);
 std::ifstream cases(argv[2]);unsigned entry,kind,abi=std::stoul(argv[3]),checks=0;
 auto step=[](unsigned x){return uint8_t((x*2)^((x&128)?83:0));};
 while(cases>>entry>>kind){
  for(unsigned x=0;x<256;++x)for(unsigned poly=0;poly<(kind==9?256u:1u);++poly){
   unsigned expected=step(x);
   if(kind==4)expected=(x&128)?step(x):x!=0;
   if(kind==5)expected=(x&128)?step(x):(x*2)&127;
   if(kind==6)expected=((x*2)&127)^((x&128)?83:0);
   if(kind==7)expected&=127;
   if(kind==8)expected=expected!=0;
   if(kind==9)expected=uint8_t((x*2)^((x&128)?poly:0));
   if(kind==10)expected=step(step(x));
   if(kind==11)expected=(x*2)^((x&128)?83:0);
   cpu.reset();auto s=cpu.snapshot();s.pc=entry;s.sp=0xfe00;s.af=(x<<8)|0x55;
   s.hl=poly;s.de=0x2468;s.bc=0x1357;s.ix=0xa55a;s.iy=0x5aa5;
   mem.word(0xfe00,0xff00);mem.word(0xfe02,x|(poly<<8));cpu.restore(s);
   unsigned instructions=0;while(cpu.pc()!=0xff00&&++instructions<5000)cpu.step();s=cpu.snapshot();
   unsigned actual=kind==11?(abi?s.de:s.hl):(abi?s.af>>8:s.hl&255);
   if(s.pc!=0xff00||s.sp!=0xfe02||s.ix!=0xa55a||actual!=expected)
    throw std::runtime_error("shift/XOR kind="+std::to_string(kind)+" x="+std::to_string(x)+" poly="+std::to_string(poly)+" expected="+std::to_string(expected)+" actual="+std::to_string(actual));
   ++checks;
  }
 }
 std::cout<<checks<<" independent shift/XOR values passed\n";
}
