typedef unsigned int u16;
__attribute__((noinline)) void increment(u16 *p) { ++*p; }
__attribute__((noinline)) void add_one(u16 *p) { *p += 1; }
__attribute__((noinline)) u16 post_increment(u16 *p) { return (*p)++; }
__attribute__((noinline)) void volatile_increment(volatile u16 *p) { ++*p; }
__attribute__((noinline)) void increment_twice(u16 *p, u16 *q) { ++*p; ++*q; }
#ifndef COMPILE_ONLY
int main(void)
{
    u16 storage[3];
    u16 n;
    for (n=0; n<513; ++n) {
        u16 i = n<257 ? n : 65535u-(n-257u);
        storage[0]=0x1357; storage[1]=i; storage[2]=0x2468;
        increment(storage+1);
        if (storage[1] != (u16)(i+1)) return 1;
        add_one(storage+1);
        if (storage[1] != (u16)(i+2)) return 2;
        if (post_increment(storage+1) != (u16)(i+2)) return 3;
        if (storage[1] != (u16)(i+3)) return 4;
        volatile_increment(storage+1);
        if (storage[1] != (u16)(i+4)) return 5;
        increment_twice(storage+1,storage+1);
        if (storage[1] != (u16)(i+6)) return 6;
        if (storage[0]!=0x1357 || storage[2]!=0x2468) return 7;
    }
    return 0;
}
#endif
