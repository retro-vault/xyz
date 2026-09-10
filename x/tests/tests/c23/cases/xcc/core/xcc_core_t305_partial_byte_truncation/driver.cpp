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
 std::ifstream cases(argv[2]);unsigned entry,kind,width,is_signed,abi=std::stoul(argv[3]),checks=0;
 while(cases>>entry>>is_signed>>width>>kind){
  for(unsigned x=0;x<256;++x){
   int value=0;
   switch(kind){
   case 0:value=int(x)+129;break;case 1:value=int(x)-129;break;
   case 2:value=x&213;break;case 3:value=x|129;break;case 4:value=x^213;break;
   case 5:value=-int(x);break;case 6:value=~int(x);break;
   case 7:value=x*2;break;case 8:value=x/2;break;case 9:value=(x*2)^85;break;
   case 10:value=x*3+129;break;case 11:value=(x+91)^183;break;
   }
   unsigned mask=(1u<<width)-1;
   int narrowed=unsigned(value)&mask;
   if(is_signed && (unsigned(narrowed)&(1u<<(width-1)))) narrowed-=1u<<width;
   unsigned expected=unsigned(narrowed)&65535;
   cpu.reset();auto s=cpu.snapshot();s.pc=entry;s.sp=0xfe00;s.af=(x<<8)|0x55;
   s.hl=0xaaaa;s.de=0x2468;s.bc=0x1357;s.ix=0xa55a;s.iy=0x5aa5;
   mem.word(0xfe00,0xff00);mem.word(0xfe02,x);cpu.restore(s);
   unsigned instructions=0;while(cpu.pc()!=0xff00&&++instructions<5000)cpu.step();s=cpu.snapshot();
   unsigned actual=abi?s.de:s.hl;
   if(s.pc!=0xff00||s.sp!=0xfe02||s.ix!=0xa55a||actual!=expected)
    throw std::runtime_error("narrow kind="+std::to_string(kind)+" sign="+std::to_string(is_signed)+" width="+std::to_string(width)+" x="+std::to_string(x)+" expected="+std::to_string(expected)+" actual="+std::to_string(actual));
   ++checks;
  }
 }
 std::cout<<checks<<" independent narrow arithmetic values passed\n";
}
