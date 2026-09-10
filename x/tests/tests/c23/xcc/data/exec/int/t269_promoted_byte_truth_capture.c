/* A narrowed view of a promoted byte must retain the original capture, even
   when its word home is omitted, its source changes, or its source dies. */
#define NOINLINE __attribute__((noinline))
static unsigned char bytes[4], ordinary_byte;
static signed char signed_byte;
static volatile unsigned char observable_byte;
static volatile unsigned input;

NOINLINE unsigned adjacent_truth(unsigned seed) {
    bytes[0]=bytes[2]=bytes[3]=0x55;
    bytes[1]=(unsigned char)seed;
    return bytes[0]!=0x55 || bytes[1]!=0 ||
           bytes[2]!=0x55 || bytes[3]!=0x55;
}
NOINLINE unsigned signed_truth(unsigned seed) {
    signed_byte=(signed char)seed;
    return signed_byte ? 19 : 7;
}
NOINLINE unsigned observable_truth(unsigned seed) {
    observable_byte=(unsigned char)seed;
    return observable_byte ? 19 : 7;
}
NOINLINE void change_source(void) {
    ordinary_byte=(unsigned char)(ordinary_byte^255);
}
NOINLINE unsigned saved_across_call(unsigned seed) {
    int saved;
    ordinary_byte=(unsigned char)seed;
    saved=ordinary_byte;
    change_source();
    return saved ? 19 : 7;
}
NOINLINE unsigned saved_across_branch(unsigned seed, unsigned branch) {
    int saved;
    ordinary_byte=(unsigned char)seed;
    saved=ordinary_byte;
    if(branch) ordinary_byte=0;
    else ordinary_byte=255;
    return saved ? 19 : 7;
}
NOINLINE unsigned saved_across_loop(unsigned seed) {
    int saved;
    unsigned i, count=0;
    ordinary_byte=(unsigned char)seed;
    saved=ordinary_byte;
    for(i=0;i<3;++i) {
        if(saved) ++count;
        change_source();
    }
    return count;
}
int main(void) {
    static const unsigned seeds[]={0,1,127,128,255,256,65535};
    static const unsigned results[]={7,19,19,19,19,7,19};
    static const unsigned booleans[]={0,1,1,1,1,0,1};
    static const unsigned counts[]={0,3,3,3,3,0,3};
    unsigned i;
    for(i=0;i<7;++i) {
        input=seeds[i];
        if(adjacent_truth(input)!=booleans[i]) return 1;
        if(signed_truth(input)!=results[i]) return 2;
        if(observable_truth(input)!=results[i]) return 3;
        if(saved_across_call(input)!=results[i]) return 4;
        if(saved_across_branch(input,0)!=results[i]) return 5;
        if(saved_across_branch(input,1)!=results[i]) return 6;
        if(saved_across_loop(input)!=counts[i]) return 7;
    }
    return 0;
}
