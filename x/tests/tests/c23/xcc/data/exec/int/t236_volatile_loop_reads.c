typedef unsigned int u16;

__attribute__((noinline)) unsigned volatile_loop_reads(unsigned count)
{
    volatile unsigned local = 0;
    unsigned sum = 0;
    unsigned i;
    for (i = 0; i < count; ++i) {
        local = i;
        sum += (local + local) + (local + local);
    }
    return sum;
}

#ifndef COMPILE_ONLY
int main(void)
{
    unsigned i;
    for (i = 0; i < 19; ++i)
        if (volatile_loop_reads(i) != 2u * i * (i - 1u)) return 1;
    return 0;
}
#endif
