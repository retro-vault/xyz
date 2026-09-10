/* Generic aggregate zeroing: complete literal coverage, dynamic initializers,
 * repeated lifetime, padding, qualifiers and over-aligned storage. */
#define NOINLINE __attribute__((noinline))
static volatile unsigned calls;
static volatile unsigned trace;
static volatile unsigned observed;

NOINLINE unsigned init_first(void) {
    ++calls;
    trace = trace * 5u + 1u;
    return 19;
}
NOINLINE unsigned init_second(void) {
    ++calls;
    trace = trace * 5u + 2u;
    return 23;
}
NOINLINE unsigned large_ordinary(unsigned seed) {
    struct envelope {
        unsigned before;
        unsigned char bytes[300];
        unsigned after;
    } value = {.before=0x125a, .bytes={[0]=3, [17]=7, [299]=11},
               .after=0x6bc2};
    unsigned i;
    for (i=0; i<300; ++i) {
        unsigned char expected=0;
        if(i==0) expected=3;
        if(i==17) expected=7;
        if(i==299) expected=11;
        if(value.bytes[i]!=expected) return 1;
    }
    value.bytes[0]=(unsigned char)seed;
    value.bytes[299]=(unsigned char)(seed+1);
    if(value.before!=0x125a || value.after!=0x6bc2) return 2;
    return 0;
}
NOINLINE unsigned repeated_blocks(void) {
    unsigned index, sum=0;
    for(index=0;index<4;++index) {
        unsigned char first[8] = {[1]=(unsigned char)(index+1)};
        sum += first[1];
        if(first[7]) return 20;
        unsigned char second[12] = {[4]=(unsigned char)(index+2)};
        sum += second[4];
        if(second[11]) return 21;
    }
    return sum!=24;
}
NOINLINE unsigned aligned_large(void) {
    _Alignas(256) unsigned char bytes[257] = {[256]=13};
    unsigned i;
    if (((unsigned)bytes & 255u) != 0) return 18;
    for (i=0;i<256;++i) if (bytes[i]!=0) return 19;
    return bytes[256]!=13;
}
NOINLINE unsigned nested_partial(void) {
    struct item { unsigned words[3]; unsigned char bytes[5]; };
    struct bundle { struct item items[2]; unsigned tail; } value = {
        .items={{.words={[1]=31}}, {.bytes={[3]=17}}}
    };
    unsigned i,j;
    for(i=0;i<2;++i) {
        for(j=0;j<3;++j) {
            unsigned expected=0;
            if(i==0 && j==1) expected=31;
            if(value.items[i].words[j]!=expected) return 3;
        }
        for(j=0;j<5;++j) {
            unsigned expected=0;
            if(i==1 && j==3) expected=17;
            if(value.items[i].bytes[j]!=expected) return 4;
        }
    }
    return value.tail;
}
NOINLINE unsigned bitfield_union(void) {
    struct bits { unsigned a:3; unsigned :2; unsigned b:5;
                  unsigned char padding[7]; } b={.a=5, .b=19};
    union choice { unsigned word; unsigned char bytes[12]; } u={.word=37};
    unsigned i;
    if(b.a!=5 || b.b!=19 || u.word!=37) return 5;
    for(i=0;i<7;++i) if(b.padding[i]!=0) return 6;
    for(i=sizeof(unsigned);i<sizeof u;++i) if(u.bytes[i]!=0) return 7;
    return 0;
}
NOINLINE unsigned initializer_calls(void) {
    struct init {unsigned first;unsigned char gap[20];unsigned second;};
    calls=0; trace=0;
    struct init value={.first=init_first(),.second=init_second()};
    unsigned i;
    if(calls!=2 || value.first!=19 || value.second!=23) return 8;
    for(i=0;i<20;++i) if(value.gap[i]) return 9;
    return 0;
}
NOINLINE unsigned volatile_member(void) {
    struct inner { volatile unsigned value; unsigned char rest[8]; };
    struct outer { struct inner members[2]; } v={.members={{.value=41}}};
    return v.members[0].value!=41 || v.members[1].value!=0 || v.members[1].rest[7]!=0;
}
NOINLINE unsigned atomic_member(void) {
    struct atom { _Atomic unsigned value; unsigned char rest[8]; };
    struct outer { struct atom members[2]; } v={.members={{.value=43}}};
    return v.members[0].value!=43 || v.members[1].value!=0 || v.members[1].rest[7]!=0;
}
NOINLINE unsigned pointer_to_volatile(void) {
    struct pointer { volatile unsigned *p; unsigned char rest[8]; } v={.p=&observed};
    *v.p=47;
    return observed!=47 || v.rest[7]!=0;
}
NOINLINE unsigned volatile_pointer(void) {
    struct pointer { unsigned * volatile p; unsigned char rest[8]; } v={0};
    return v.p!=0 || v.rest[7]!=0;
}
NOINLINE unsigned complete_calls(void) {
    struct pair { unsigned first, second; } value = {
        .first=init_first(), .second=init_second()
    };
    return value.first!=19 || value.second!=23;
}
NOINLINE unsigned complete_literals(unsigned index) {
    unsigned char bytes[128] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128};
    unsigned nested[2][3] = {{1,2,3},{4,5,6}};
    struct dense { unsigned first; unsigned char rest[4]; } value = {7,{1,2,3,4}};
    union full {unsigned long first, second;} u={.second=13};
    if (nested[1][2]!=6 || value.first!=7 || value.rest[3]!=4 || u.second!=13)
        return 255;
    return bytes[index];
}
NOINLINE unsigned designated_duplicates(void) {
    unsigned char bytes[8] = {[1]=3,[1]=7,[5]=11};
    return bytes[0]!=0 || bytes[1]!=7 || bytes[5]!=11 || bytes[7]!=0;
}
int main(void) {
    unsigned i;
    for(i=0;i<4;++i) if(large_ordinary(i*67u)) return 10;
    if(repeated_blocks()) return 19;
    if(aligned_large()) return 18;
    if(nested_partial()) return 11;
    if(bitfield_union()) return 12;
    if(initializer_calls()) return 13;
    if(volatile_member()) return 14;
    if(atomic_member()) return 15;
    if(pointer_to_volatile()) return 16;
    if(volatile_pointer()) return 17;
    if(complete_calls()) return 22;
    for(i=0;i<128;++i) if(complete_literals(i)!=i+1) return 23;
    if(designated_duplicates()) return 24;
    return 0;
}
