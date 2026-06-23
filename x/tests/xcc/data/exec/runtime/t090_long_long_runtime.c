#include "xcc_exec_test.h"

static unsigned long long ull_add(unsigned long long a, unsigned long long b) {
    return a + b;
}

static long long sll_mul(long long a, long long b) {
    return a * b;
}

static long long sll_div(long long a, long long b) {
    return a / b;
}

static long long sll_mod(long long a, long long b) {
    return a % b;
}

static unsigned long long ull_shl(unsigned long long a, int count) {
    return a << count;
}

static unsigned long long ull_shr(unsigned long long a, int count) {
    return a >> count;
}

static int sll_lt(long long a, long long b) {
    return a < b;
}

static unsigned long long ull_band(unsigned long long a,
                                   unsigned long long b) {
    return a & b;
}

static unsigned long long ull_bor(unsigned long long a,
                                  unsigned long long b) {
    return a | b;
}

static unsigned long long ull_bxor(unsigned long long a,
                                   unsigned long long b) {
    return a ^ b;
}

static unsigned long long ull_bnot(unsigned long long a) {
    return ~a;
}

static long long sll_neg(long long a) {
    return -a;
}

int main(void) {
    unsigned long long ua = 0x0000010000003039ULL;
    unsigned long long ub = 0x0000000000100141ULL;
    unsigned long long uc = 0x123456789ABCDEF0ULL;
    unsigned long long ud = 0x00FF00FF00FF00FFULL;
    if (ull_add(ua, ub) != 0x000001000010317AULL) return 1;
    if (sll_mul(300000LL, 7000LL) != 2100000000LL) return 2;
    if (sll_div(-60000000003LL, 3000LL) != -20000000LL) return 3;
    if (sll_mod(-60000000003LL, 3000LL) != -3LL) return 4;
    if (ull_shl(((unsigned long long)1 << 32) + 5ULL, 3) !=
        0x0000000800000028ULL) return 5;
    if (ull_shr(((unsigned long long)1 << 48) + 0x1234ULL, 4) !=
        0x0000100000000123ULL) return 6;
    if (!sll_lt(-5LL, 2LL)) return 7;
    if (sll_lt(7LL, 7LL)) return 8;
    if (ull_band(uc, ud) != 0x0034007800BC00F0ULL) return 9;
    if (ull_bor(uc, ud) != 0x12FF56FF9AFFDEFFULL) return 10;
    if (ull_bxor(uc, ud) != 0x12CB56879A43DE0FULL) return 11;
    if (ull_bnot(uc) != 0xEDCBA9876543210FULL) return 12;
    if (sll_neg(0x123456789abcLL) != (-0x123456789abcLL)) return 13;

    return 0;
}
