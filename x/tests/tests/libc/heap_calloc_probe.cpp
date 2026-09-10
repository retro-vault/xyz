#include <xz80/xz80.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>
struct memory : xz80::IMemory {
 std::array<uint8_t,65536> data{};
 unsigned writes=0; unsigned low=0x4000,high=0xc001;
 uint8_t read(uint16_t a) const noexcept override{return data[a];}
 void write(uint16_t a,uint8_t v) noexcept override{data[a]=v;if(a>=low&&a<high)++writes;}
 void word(unsigned a,unsigned v){data[a]=v;data[a+1]=v>>8;}
 unsigned word(unsigned a){return data[a]|unsigned(data[a+1])<<8;}
};
struct ports : xz80::IPorts {uint8_t in(uint16_t) noexcept override{return 0;}void out(uint16_t,uint8_t) noexcept override{}};
int main(int argc,char**argv){
 if(argc!=4)return 2;
 memory mem;ports io;xz80::cpu cpu(mem,io);
 std::ifstream binary(argv[1],std::ios::binary);binary.read((char*)mem.data.data(),65536);
 auto image=mem.data; std::map<std::string,unsigned> syms;
 std::ifstream map(argv[2]);std::string line;std::regex sym("^([0-9A-Fa-f]{8}) ([^ ]+)");std::smatch match;
 while(std::getline(map,line))if(std::regex_search(line,match,sym))syms[match[2]]=std::stoul(match[1],nullptr,16);
 const bool isolated=std::string(argv[3])=="isolated"; unsigned long long checks=0;
 auto need=[&](bool p,std::string message){if(!p)throw std::runtime_error(message);++checks;};
 auto start=[&](std::string name,unsigned a,unsigned b,unsigned c=0){
  cpu.reset();auto state=cpu.snapshot();state.pc=syms.at(name);state.sp=0xfe00;state.hl=a;state.de=b;
  state.bc=0x6bad;state.af=0x1234;state.ix=0xa55a;state.iy=0x5aa5;cpu.restore(state);
  mem.word(0xfe00,0xff00);mem.word(0xfe02,c);mem.writes=0;
 };
 auto execute=[&](std::string name,unsigned a,unsigned b,unsigned c=0){
  start(name,a,b,c);unsigned steps=0,cycles=0;
  while(cpu.pc()!=0xff00&&++steps<1000000)cycles+=cpu.step();
  need(cpu.pc()==0xff00,name+" did not return");
  const unsigned expected_sp=name=="_heap_init_arena"?0xfe04:0xfe02;
  need(cpu.snapshot().sp==expected_sp,name+" stack mismatch");
  if(name=="_heap_init_arena")
   need(cpu.snapshot().ix==0xa55a&&cpu.snapshot().iy==0x5aa5,
        "heap initialization changed preserved registers");
  return std::pair<unsigned,unsigned>(cpu.snapshot().de,cycles);
 };
 if(isolated){
  auto product=[&](unsigned a,unsigned b){
   start("_calloc",a,b);unsigned steps=0;
   while(cpu.pc()!=syms.at("_malloc")&&cpu.pc()!=0xff00&&++steps<2000)cpu.step();
   uint64_t expected=uint64_t(a)*b;bool called=cpu.pc()==syms.at("_malloc");
   need(called==(expected<=65535),"product overflow decision "+std::to_string(a)+"*"+std::to_string(b));
   if(called)need(cpu.snapshot().hl==expected,"wrong malloc size");
   else need(cpu.pc()==0xff00&&cpu.snapshot().de==0,"overflow return mismatch");
  };
  for(unsigned a=0;a<65536;++a){
   for(unsigned b:{0u,1u,2u,3u,7u,255u,256u,257u,32767u,32768u,65535u})product(a,b);
   if(a){unsigned limit=65535/a;product(a,limit);if(limit<65535)product(a,limit+1);if(limit)product(a,limit-1);}
  }
  for(auto pair:std::vector<std::pair<unsigned,unsigned>>{{0,65535},{65535,0},{1,1},{1,2},{1,32},{32,1},{16,16},{100,7},{1,1024},{1024,1},{1,32768}}){
   for(bool fail:{false,true}){
    unsigned total=pair.first*pair.second;std::fill(mem.data.begin()+0x3fff,mem.data.begin()+0xc002,0xcc);
    start("_calloc",pair.first,pair.second);unsigned calls=0,cycles=0,steps=0;
    while(cpu.pc()!=0xff00&&++steps<1000000){
     if(cpu.pc()==syms.at("_malloc")){
      ++calls;auto state=cpu.snapshot();need(state.hl==total,"clear malloc size");
      state.pc=mem.word(state.sp);state.sp+=2;state.de=fail||total==0?0:0x4000;
      state.hl=0xbeef;state.bc=0xdead;state.af=0xffff;cpu.restore(state);cycles+=10;
     }else cycles+=cpu.step();
    }
    auto state=cpu.snapshot();need(state.pc==0xff00&&state.sp==0xfe02&&calls==1,"clear return/stack/call count");
    need(state.ix==0xa55a&&state.iy==0x5aa5,"callee-save corruption");
    need(state.de==(fail||total==0?0:0x4000),"calloc result pointer");
    need(mem.writes==(fail||total==0?0:total),"wrong exact clear write count");
    need(mem.data[0x3fff]==0xcc&&mem.data[0x4000+total]==0xcc,"clear crossed allocation boundary");
    if(!fail)for(unsigned i=0;i<total;++i)need(mem.data[0x4000+i]==0,"nonzero payload");
    if(!fail)std::cout<<"CYCLES "<<pair.first<<' '<<pair.second<<' '<<cycles<<'\n';
   }
  }
 }else{
  auto reset=[&](){mem.data=image;};
  // Exercise the actual default-heap setup through a controlled platform hook.
  // The raw custom-heap primitive below deliberately keeps the supplied base.
  for(auto bounds:std::vector<std::pair<unsigned,unsigned>>{{0x4000,0xc000},{0x4001,0xc000},{0x4007,0x4014},{0x4001,0x4008},{0x5000,0x4000},{0xffff,0xffff},{0,0x1000}}){
   reset();unsigned hook=syms.at("_heap_region");
   need(mem.data[hook]==0x21&&mem.data[hook+3]==0x11,"unexpected platform hook fixture");
   mem.word(hook+1,bounds.first);mem.word(hook+4,bounds.second);
   unsigned header=(bounds.first+1)&0xfffe;
   bool fits=header!=0&&bounds.second>=header+12;
   unsigned result=execute("_malloc",3,0).first;
   need(result==(fits?header+8:0),"default platform alignment/bounds");
   unsigned descriptor=syms.at("__libc_default_heap");
   need(mem.word(descriptor+2)==header&&mem.word(descriptor+4)==bounds.second,"default region descriptor bounds");
   if(fits){need(!(result&1),"unaligned default payload");execute("_free",result,0);}
  }
  reset();auto huge=execute("_malloc",65535,0);std::cout<<"MALLOC65535 "<<huge.first<<'\n';
  reset();unsigned p=execute("_malloc",16,0).first;need(p!=0,"initial allocation failed");
  for(unsigned i=0;i<16;++i)mem.data[p+i]=uint8_t(i+0x30);
  auto before=mem.data;unsigned changed=execute("_realloc",p,65535).first;
  std::cout<<"REALLOC65535 "<<changed<<" OLD_HEADER "<<mem.word(p-8)<<'\n';
  need(huge.first==0&&changed==0,"size alignment overflow accepted");
  need(std::equal(before.begin()+p-8,before.begin()+p+16,mem.data.begin()+p-8),"failed realloc modified old block");
  for(auto pair:std::vector<std::pair<unsigned,unsigned>>{{0,0},{0,65535},{65535,0},{1,65535},{65535,1},{257,255},{256,256},{65535,65535}}){
   reset();need(execute("_calloc",pair.first,pair.second).first==0,"calloc oversized or zero allocation accepted");
  }
  for(unsigned n:{1u,2u,3u,7u,255u,256u,511u,1024u,4096u}){
   reset();p=execute("_malloc",n,0).first;need(p&&!(p&1),"malloc alignment");
   std::fill(mem.data.begin()+p,mem.data.begin()+p+n,0xa5);execute("_free",p,0);
   auto result=execute("_calloc",1,n);p=result.first;need(p&&!(p&1),"calloc allocation/alignment");
   for(unsigned i=0;i<n;++i)need(mem.data[p+i]==0,"calloc did not clear reused payload");
   std::cout<<"HEAP_CYCLES 1 "<<n<<' '<<result.second<<'\n';
   for(unsigned i=0;i<n;++i)mem.data[p+i]=uint8_t(i*17+3);
   unsigned q=execute("_realloc",p,n+12).first;need(q!=0,"realloc growth failed");
   for(unsigned i=0;i<n;++i)need(mem.data[q+i]==uint8_t(i*17+3),"realloc growth lost payload");
   execute("_free",q,0);
  }
  for(unsigned first:{0x4000u,0x4001u}){
   reset();execute("_heap_init_arena",0x3100,first,0x4200);
   unsigned header=first,available=0x4200-header-8;
   need(mem.word(0x3100)==header&&mem.word(0x3102)==first,"raw custom arena bounds");
   p=execute("_allocate",0x3100,available&~1u).first;
   need(p==header+8,"raw custom arena exact fit");
   need(mem.word(header)==available,"raw custom arena retained size");
   need(execute("_allocate",0x3100,1).first==0,"arena exceeded its limit");
  }
  for(auto bounds:std::vector<std::pair<unsigned,unsigned>>{{0x4001,0x4008},{0x4000,0x4007},{0x5000,0x4000},{0xffff,0xffff},{0,0x1000}}){
   reset();auto prior=mem.data;
   execute("_heap_init_arena",0x3100,bounds.first,bounds.second);
   need(mem.word(0x3100)==0,"invalid arena fabricated a block");
   need(execute("_allocate",0x3100,1).first==0,"empty arena allocated");
   need(std::equal(prior.begin(),prior.begin()+0x3000,mem.data.begin()),"invalid arena changed low memory");
   need(std::equal(prior.begin()+0x4000,prior.begin()+0xe000,mem.data.begin()+0x4000),"invalid arena wrote a header");
  }
  reset();execute("_heap_init_arena",0x3100,0xd000,0xe000);
  need(execute("_allocate",0x3100,65535).first==0,"custom heap overflow accepted");
  p=execute("_allocate",0x3100,101).first;need(p>=0xd008&&p<0xe000&&mem.word(p-2)==0x3100,"custom heap owner");
  execute("_free",p,0);need(mem.word(0xd002)==1,"free lost owning heap");
  reset();p=execute("_aligned_alloc",32,64).first;need(p&&!(p&31),"aligned allocation");
  std::fill(mem.data.begin()+p,mem.data.begin()+p+64,0x57);before=mem.data;
  need(execute("_realloc",p,65535).first==0,"aligned realloc overflow accepted");
  need(std::equal(before.begin()+p-6,before.begin()+p+64,mem.data.begin()+p-6),"failed aligned realloc changed data");
  execute("_free",p,0);
 }
 std::cout<<checks<<" heap/calloc checks passed\n";
}
