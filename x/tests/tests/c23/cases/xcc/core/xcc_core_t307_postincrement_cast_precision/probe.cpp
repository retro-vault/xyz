#include <xz80/xz80.h>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
struct memory:xz80::IMemory{
 std::array<uint8_t,65536>d{};unsigned cursor=0;mutable unsigned source_reads=0,cursor_reads=0;unsigned cursor_writes=0;mutable std::string trace;
 uint8_t read(uint16_t a)const noexcept override{if(a>=0xc000&&a<0xc002){++source_reads;trace+='S';}if(a==cursor||a==cursor+1){++cursor_reads;trace+='C';}return d[a];}
 void write(uint16_t a,uint8_t b)noexcept override{d[a]=b;if(a==cursor||a==cursor+1){++cursor_writes;trace+='W';}}
 void word(unsigned a,unsigned b){d[a]=b;d[a+1]=b>>8;}
};
struct ports:xz80::IPorts{uint8_t in(uint16_t)noexcept override{return 0;}void out(uint16_t,uint8_t)noexcept override{}};
int main(int argc,char**argv){memory m;ports p;xz80::cpu cpu(m,p);std::ifstream b(argv[1],std::ios::binary);b.read((char*)m.d.data(),65536);std::ifstream map(argv[2]);std::map<std::string,unsigned>s;std::string l;std::regex re("^([0-9A-Fa-f]{8}) ([^ ]+)");std::smatch mt;while(getline(map,l))if(regex_search(l,mt,re))s[mt[2]]=std::stoul(mt[1],nullptr,16);int abi=std::stoi(argv[3]);bool force=std::stoi(argv[4]);m.cursor=s.at("_cursor");unsigned checks=0,fullbyte_fusions=0;
auto call=[&](std::string name){cpu.reset();auto st=cpu.snapshot();st.pc=s.at(name);st.sp=0xfef0;st.hl=0xc000;st.de=0x7351;st.bc=0xa239;st.ix=0x8675;st.iy=0x9512;cpu.restore(st);m.word(0xfef0,0xff00);m.word(0xfef2,0xc000);m.trace.clear();m.source_reads=m.cursor_reads=m.cursor_writes=0;unsigned steps=0;while(cpu.pc()!=0xff00&&++steps<30000)cpu.step();auto got=cpu.snapshot();if(cpu.pc()!=0xff00)throw std::runtime_error("call did not return");return unsigned(abi?got.de:got.hl);};
for(unsigned src=0;src<4;++src)for(unsigned sign=0;sign<2;++sign)for(unsigned w=1+sign;w<=8;++w){std::string name="_f"+std::to_string(src)+"_"+std::to_string(sign)+"_"+std::to_string(w);unsigned mask=(1u<<w)-1,step=src<2?2:1;for(unsigned i=0;i<1024;++i){unsigned raw=(i*257u)&65535;unsigned x=raw&mask;if(sign&&(x&(1u<<(w-1))))x|=~mask;unsigned expected=(x+0xc000+step)&65535;m.word(0xc000,raw);unsigned got=call(name);if(got!=expected||((src&1)&&m.source_reads!=step)){std::cerr<<name<<" input="<<raw<<" got="<<got<<" expected="<<expected<<" source_reads="<<m.source_reads<<" expected_volatile_reads="<<step<<'\n';return 1;}if(src==0&&w==8&&force){if(m.source_reads!=1){std::cerr<<name<<" lost ordinary full-byte shortcut\n";return 1;}++fullbyte_fusions;}++checks;}}
for(unsigned i=0;i<256;++i){m.word(0xc000,i*257);m.word(m.cursor,0xc000);unsigned got=call("_cursor_case");if(got!=0xc002+i||m.source_reads!=2||m.cursor_reads!=6||m.cursor_writes!=2||m.trace!="CCWWCCSSCC"){std::cerr<<"cursor case i="<<i<<" got="<<got<<" trace="<<m.trace<<'\n';return 1;}++checks;}
for(auto name:{"_local_alias","_global_alias"}){unsigned got=call(name);if(got!=0){std::cerr<<name<<" loaded the cursor before its update: "<<got<<'\n';return 1;}++checks;}
std::cout<<checks<<" postincrement precision/access cases passed; "<<fullbyte_fusions<<" ordinary byte projections verified\n";}
