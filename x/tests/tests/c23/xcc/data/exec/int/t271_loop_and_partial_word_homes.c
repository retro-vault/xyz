/* Local storage stays coherent across partial writes and nested loops that
   carry a byte value without mentioning it in their inner body. */
#define NOINLINE __attribute__((noinline))
static unsigned char a[32], b[32], c[32], d[32];
static volatile unsigned seed=0x1234;

NOINLINE void initialize_word(unsigned *p) { *p=seed; }
NOINLINE unsigned low_partial(void) {
    unsigned value=seed;
    *(unsigned char *)&value=0x56;
    return value;
}
NOINLINE unsigned high_partial(void) {
    unsigned value=seed;
    *((unsigned char *)&value+1)=0x56;
    return value;
}
NOINLINE unsigned hidden_partial(void) {
    unsigned value;
    initialize_word(&value);
    *(unsigned char *)&value=0x56;
    return value;
}
NOINLINE unsigned hidden_high_partial(void) {
    unsigned value;
    initialize_word(&value);
    *((unsigned char *)&value+1)=0x56;
    return value;
}
NOINLINE unsigned modified_counter(void) {
    unsigned i, value=5, sum=0;
    unsigned char *low=(unsigned char *)&value;
    for(i=0;i<17;++i,value+=2) {
        if(value==11) *low=13;
        sum+=value;
    }
    return sum^value;
}
NOINLINE void emit_chunks(unsigned limit) {
    unsigned i=0, j, length;
    unsigned char state=7, value;
    while(i<limit) {
        state=(unsigned char)(state*5u+3u);
        length=(state&3u)+1u;
        value=state>>3;
        for(j=0;j<length && i<limit;++j) {
            a[i]=value;
            b[i]=(unsigned char)(value+1);
            c[i]=(unsigned char)(value^0x31);
            d[i]=(unsigned char)(value+5);
            ++i;
        }
    }
}
int main(void) {
    static const unsigned char expected[]={4,4,4,24,24,25,29,29,29,29,19,19,19,0,0,3,17,17,17,17,25,25,25,1,1,6,30,30,30,30,24,24};
    unsigned i;
#ifndef LOOP_ONLY
    if(low_partial()!=0x1256) return 1;
    if(high_partial()!=0x5634) return 2;
    if(hidden_partial()!=0x1256) return 3;
    if(hidden_high_partial()!=0x5634) return 4;
    if(modified_counter()!=(385u^41u)) return 5;
#endif
    seed=32;
    emit_chunks(seed);
    for(i=0;i<32;++i) {
        if(a[i]!=expected[i]) return 6;
        if(b[i]!=(unsigned char)(expected[i]+1)) return 7;
        if(c[i]!=(unsigned char)(expected[i]^0x31)) return 8;
        if(d[i]!=(unsigned char)(expected[i]+5)) return 9;
    }
    return 0;
}
