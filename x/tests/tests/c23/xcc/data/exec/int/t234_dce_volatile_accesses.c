#ifndef TEST_CASE
#define TEST_CASE 0
#endif

typedef unsigned char u8;
volatile u8 observable_byte;
volatile signed char observable_signed_byte;

#if TEST_CASE == 0 || TEST_CASE == 1
int volatile_mask(volatile u8 *p)
{
    return ((unsigned)*p | 0x100u) < 256u;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 2
unsigned volatile_shift(volatile unsigned *p)
{
    return (*p >> 9) >> 9;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 3
int volatile_repeat(volatile u8 *p)
{
    unsigned first = *p;
    unsigned second = *p;
    return ((first | second) | 0x100u) < 256u;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 4
int volatile_direct(void)
{
    unsigned value = observable_byte;
    return (value | 0x100u) < 256u;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 5
void volatile_unused(volatile u8 *p)
{
    *p;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 6
void volatile_local_writes(void)
{
    volatile u8 local;
    local = 3;
    local = 7;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 7
int volatile_direct_repeat(void)
{
    unsigned first = observable_byte;
    unsigned second = observable_byte;
    return ((first | second) | 0x100u) < 256u;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 8
int volatile_signed_direct(void)
{
    int value = observable_signed_byte;
    return (value | 0x100) == 0;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 9
int volatile_direct_branch(void)
{
    return observable_byte ? 3 : 7;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 10
unsigned volatile_local_repeat(void)
{
    volatile u8 local = 5;
    return local + local;
}
#endif

/* These compile-only cases exercise the implicit observable-access contract
   of SFR objects, without adding host-specific attributes to the C runtime
   payload. Both immediate and full-width Z80 port addressing are covered. */
#if TEST_CASE >= 11 && TEST_CASE <= 16
[[sdcc::sfr(0x23)]] u8 observable_port;
[[sdcc::sfr(0x1234)]] u8 observable_wide_port;
#endif

#if TEST_CASE == 11
unsigned sfr_range_fold(void)
{
    unsigned value = observable_wide_port;
    return (value | 0x100u) < 256u;
}
#endif

#if TEST_CASE == 12
unsigned sfr_repeat_range_fold(void)
{
    unsigned first = observable_port;
    unsigned second = observable_port;
    return ((first | second) | 0x100u) < 256u;
}
#endif

#if TEST_CASE == 13
void sfr_unused(void)
{
    observable_wide_port;
}
#endif

#if TEST_CASE == 14
unsigned sfr_zero_mask(void)
{
    return observable_port & 0;
}
#endif

#if TEST_CASE == 15
unsigned sfr_nested_mask(void)
{
    return ((unsigned)observable_wide_port & 0xf3u) & 0x3fu;
}
#endif

#if TEST_CASE == 16
unsigned sfr_nested_zero_mask(void)
{
    return ((unsigned)observable_port & 0xf0u) & 0x0fu;
}
#endif

#if TEST_CASE == 17
void volatile_discarded_expressions(void)
{
    observable_byte;
    (observable_byte, observable_byte);
}
#endif

#if TEST_CASE == 0
int main(void)
{
    unsigned value = 0xffffu;
    observable_byte = 0xa7;
    if (volatile_mask(&observable_byte)) return 1;
    if (volatile_shift(&value)) return 2;
    if (volatile_repeat(&observable_byte)) return 3;
    if (volatile_direct()) return 4;
    volatile_unused(&observable_byte);
    volatile_local_writes();
    if (volatile_direct_repeat()) return 5;
    if (observable_byte != 0xa7 || value != 0xffffu) return 6;
    observable_signed_byte = -1;
    if (volatile_signed_direct()) return 7;
    if (volatile_direct_branch() != 3) return 8;
    observable_byte = 0;
    if (volatile_direct_branch() != 7) return 9;
    if (volatile_local_repeat() != 10) return 10;
    return 0;
}
#endif
