/* Every controlling expression is evaluated once, including empty switches. */
#ifndef TEST_CASE
#define TEST_CASE 1
#endif
#if TEST_CASE == 5
[[sdcc::sfr(0x80)]] unsigned char control_port;
#elif TEST_CASE == 8 || TEST_CASE == 9
_Atomic unsigned control;
#else
volatile unsigned control;
#endif
volatile unsigned calls;
__attribute__((noinline)) unsigned sample(void) { ++calls; return 2; }
unsigned choose(unsigned input)
{
#if TEST_CASE == 2
    volatile unsigned local = input;
    switch (local) {
#elif TEST_CASE == 3 || TEST_CASE == 9
    switch (control) { }
    return 44;
#elif TEST_CASE == 4
    switch (sample()) {
#elif TEST_CASE == 5
    switch (control_port) {
#elif TEST_CASE == 6
    switch (sample()) { }
    return 44;
#elif TEST_CASE == 7
    {
    volatile unsigned *pointer = &control;
    switch (*pointer) {
#else
    switch (control) {
#endif
#if TEST_CASE != 3 && TEST_CASE != 6 && TEST_CASE != 9
    case 0: return 41;
    case 1: return 42;
    case 2: return 43;
    default: return 44;
    }
#endif
#if TEST_CASE == 7
    }
#endif
}
#ifndef COMPILE_ONLY
int main(void)
{
    unsigned i;
    for (i=0; i<257; ++i) {
        control=i;
        if (choose(i) != (i<3 ? 41+i : 44)) return 1;
    }
    return 0;
}
#endif
