#ifndef TEST_CASE
#define TEST_CASE 1
#endif
unsigned increment(unsigned input)
{
#if TEST_CASE == 2
    volatile unsigned char local = input;
    if (++local) return 9;
    return 7;
#elif TEST_CASE == 3
    volatile unsigned local = input;
    local += 1;
    return input != 0;
#elif TEST_CASE == 4
    volatile unsigned local = input;
    return (local = local);
#elif TEST_CASE == 5
    volatile unsigned char local;
    return (local = input);
#elif TEST_CASE == 6
    volatile unsigned local = input;
    return (local = local + 1);
#elif TEST_CASE == 7
    volatile unsigned local = input;
    return ++local;
#elif TEST_CASE == 8
    volatile unsigned local = input;
    return local++;
#elif TEST_CASE == 9
    volatile unsigned char local = input;
    return local++;
#elif TEST_CASE == 10
    struct { volatile unsigned value; } local;
    local.value = input;
    ++local.value;
    return input != 0;
#elif TEST_CASE == 11
    unsigned local = input;
    volatile unsigned *alias = &local;
    ++*alias;
    return input != 0;
#else
    volatile unsigned local = input;
    ++local;
    return input != 0;
#endif
}
#ifndef COMPILE_ONLY
int main(void)
{
    unsigned n;
    for (n=0; n<513; ++n) {
        unsigned i = n<257 ? n : 65535u-(n-257u);
        if (increment(i) != (i != 0)) return 1;
    }
    return 0;
}
#endif
