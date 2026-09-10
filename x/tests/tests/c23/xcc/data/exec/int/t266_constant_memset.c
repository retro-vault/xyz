/* Constant memory builtins retain the external function's C contract. */
extern void *memset(void *, int, unsigned int);
#ifndef TEST_COUNT
#define TEST_COUNT 257
#endif
static unsigned char bytes[520];
static unsigned destination_calls, value_calls, count_calls;
volatile unsigned control;
static union { void *pointer; unsigned char representation[2]; } alias;
__attribute__((noinline)) static void *destination(void)
{ ++destination_calls; return bytes+3; }
__attribute__((noinline)) static int byte_value(unsigned value)
{ ++value_calls; return (int)value; }
unsigned exercise(unsigned seed)
{
    unsigned i;
    void *result;
    destination_calls=value_calls=count_calls=0;
    for(i=0;i<520;++i) bytes[i]=0xa5;
    result=memset(destination(), byte_value(seed), (count_calls++, TEST_COUNT));
    if(result!=bytes+3 || destination_calls!=1 || value_calls!=1 || count_calls!=1)
        return 1;
    for(i=0;i<520;++i) {
        unsigned char expected=(i>=3 && i<3+TEST_COUNT) ? (unsigned char)seed : 0xa5;
        if(bytes[i]!=expected) return 2;
    }
    alias.pointer=&alias.pointer;
    result=memset(alias.pointer, 0, sizeof alias.pointer);
    if(result!=&alias.pointer || alias.representation[0] || alias.representation[1])
        return 3;
    bytes[5]=(unsigned char)seed;
    result=memset(bytes+4, bytes[5], 16);
    if(result!=bytes+4) return 4;
    for(i=4;i<20;++i) if(bytes[i]!=(unsigned char)seed) return 5;
    memset(bytes+32, seed, 1);
    if(bytes[32]!=(unsigned char)seed) return 6;
    control=seed;
    memset(bytes, control, 0);
    return 0;
}
#ifndef COMPILE_ONLY
int main(void)
{
    unsigned i;
    for(i=0;i<4;++i) if(exercise(i*257u)) return 1;
    if(exercise(65535u)) return 2;
    return 0;
}
#endif
