/*
 * Numeric benchmark helpers for fixed8_8, fixed16_16, fixed24_8, float,
 * and double.
 *
 * Each benchmark is compiled five times by defining NUM_KIND:
 *   1 fixed8_8   raw int16
 *   2 fixed16_16 raw int32
 *   3 fixed24_8  raw int32
 *   4 float
 *   5 double
 */
#ifndef XYZ_NUMERIC_BENCH_H
#define XYZ_NUMERIC_BENCH_H

#define NUM_KIND_FIXED8_8   1
#define NUM_KIND_FIXED16_16 2
#define NUM_KIND_FIXED24_8  3
#define NUM_KIND_FLOAT      4
#define NUM_KIND_DOUBLE     5

#ifndef NUM_KIND
#define NUM_KIND NUM_KIND_FIXED8_8
#endif

#if NUM_KIND == NUM_KIND_FIXED8_8
typedef int num_t;
extern num_t fixed8_8_abs(num_t x);
extern num_t fixed8_8_add(num_t a, num_t b);
extern int   fixed8_8_cmp(num_t a, num_t b);
extern num_t fixed8_8_div(num_t a, num_t b);
extern num_t fixed8_8_from_int(int x);
extern num_t fixed8_8_mul(num_t a, num_t b);
extern num_t fixed8_8_neg(num_t x);
extern num_t fixed8_8_sub(num_t a, num_t b);
extern int   fixed8_8_to_int(num_t x);
#define NUM_ABS(x)       fixed8_8_abs((x))
#define NUM_ADD(a,b)     fixed8_8_add((a),(b))
#define NUM_CMP(a,b)     fixed8_8_cmp((a),(b))
#define NUM_DIV(a,b)     fixed8_8_div((a),(b))
#define NUM_FROM_INT(x)  fixed8_8_from_int((x))
#define NUM_MUL(a,b)     fixed8_8_mul((a),(b))
#define NUM_NEG(x)       fixed8_8_neg((x))
#define NUM_SUB(a,b)     fixed8_8_sub((a),(b))
#define NUM_TO_INT(x)    fixed8_8_to_int((x))
#elif NUM_KIND == NUM_KIND_FIXED16_16
typedef long num_t;
extern num_t fixed16_16_abs(num_t x);
extern num_t fixed16_16_add(num_t a, num_t b);
extern int   fixed16_16_cmp(num_t a, num_t b);
extern num_t fixed16_16_div(num_t a, num_t b);
extern num_t fixed16_16_from_int(int x);
extern num_t fixed16_16_mul(num_t a, num_t b);
extern num_t fixed16_16_neg(num_t x);
extern num_t fixed16_16_sub(num_t a, num_t b);
extern int   fixed16_16_to_int(num_t x);
#define NUM_ABS(x)       fixed16_16_abs((x))
#define NUM_ADD(a,b)     fixed16_16_add((a),(b))
#define NUM_CMP(a,b)     fixed16_16_cmp((a),(b))
#define NUM_DIV(a,b)     fixed16_16_div((a),(b))
#define NUM_FROM_INT(x)  fixed16_16_from_int((x))
#define NUM_MUL(a,b)     fixed16_16_mul((a),(b))
#define NUM_NEG(x)       fixed16_16_neg((x))
#define NUM_SUB(a,b)     fixed16_16_sub((a),(b))
#define NUM_TO_INT(x)    fixed16_16_to_int((x))
#elif NUM_KIND == NUM_KIND_FIXED24_8
typedef long num_t;
extern num_t fixed24_8_abs(num_t x);
extern num_t fixed24_8_add(num_t a, num_t b);
extern int   fixed24_8_cmp(num_t a, num_t b);
extern num_t fixed24_8_div(num_t a, num_t b);
extern num_t fixed24_8_from_int(int x);
extern num_t fixed24_8_mul(num_t a, num_t b);
extern num_t fixed24_8_neg(num_t x);
extern num_t fixed24_8_sub(num_t a, num_t b);
extern int   fixed24_8_to_int(num_t x);
#define NUM_ABS(x)       fixed24_8_abs((x))
#define NUM_ADD(a,b)     fixed24_8_add((a),(b))
#define NUM_CMP(a,b)     fixed24_8_cmp((a),(b))
#define NUM_DIV(a,b)     fixed24_8_div((a),(b))
#define NUM_FROM_INT(x)  fixed24_8_from_int((x))
#define NUM_MUL(a,b)     fixed24_8_mul((a),(b))
#define NUM_NEG(x)       fixed24_8_neg((x))
#define NUM_SUB(a,b)     fixed24_8_sub((a),(b))
#define NUM_TO_INT(x)    fixed24_8_to_int((x))
#elif NUM_KIND == NUM_KIND_FLOAT
typedef float num_t;
#define NUM_ABS(x)       (((x) < (num_t)0) ? -(x) : (x))
#define NUM_ADD(a,b)     ((a) + (b))
#define NUM_CMP(a,b)     (((a) < (b)) ? -1 : (((a) > (b)) ? 1 : 0))
#define NUM_DIV(a,b)     ((a) / (b))
#define NUM_FROM_INT(x)  ((num_t)(x))
#define NUM_MUL(a,b)     ((a) * (b))
#define NUM_NEG(x)       (-(x))
#define NUM_SUB(a,b)     ((a) - (b))
#define NUM_TO_INT(x)    ((int)(x))
#elif NUM_KIND == NUM_KIND_DOUBLE
typedef double num_t;
#define NUM_ABS(x)       (((x) < (num_t)0) ? -(x) : (x))
#define NUM_ADD(a,b)     ((a) + (b))
#define NUM_CMP(a,b)     (((a) < (b)) ? -1 : (((a) > (b)) ? 1 : 0))
#define NUM_DIV(a,b)     ((a) / (b))
#define NUM_FROM_INT(x)  ((num_t)(x))
#define NUM_MUL(a,b)     ((a) * (b))
#define NUM_NEG(x)       (-(x))
#define NUM_SUB(a,b)     ((a) - (b))
#define NUM_TO_INT(x)    ((int)(x))
#else
#error "unsupported NUM_KIND"
#endif

static unsigned int
bench_seed_word(void)
{
    unsigned int s;

    s = *((volatile unsigned int *)0xff10u);
    s ^= 0x4d35u;
    s += 0x1234u;
    return s;
}

static int
bench_small(unsigned int salt)
{
    unsigned int x;

    x = bench_seed_word();
    x ^= (unsigned int)(salt * 37u);
    x ^= (unsigned int)(x >> 5);
    return (int)((x & 7u) + 1u);
}

static unsigned int
bench_mix(unsigned int acc, int value)
{
    acc ^= (unsigned int)value;
    acc = (unsigned int)((acc << 5) | (acc >> 11));
    acc += (unsigned int)(value * 17 + 0x9e37u);
    return acc;
}

static num_t
bench_frac(int n, int d)
{
    return NUM_DIV(NUM_FROM_INT(n), NUM_FROM_INT(d));
}

static int
bench_finish(num_t x, unsigned int salt)
{
    unsigned int acc;

    acc = bench_mix((unsigned int)(0x6000u + salt), NUM_TO_INT(x));
    acc = bench_mix(acc, NUM_CMP(x, NUM_FROM_INT(0)));
    return (int)(acc & 0x7fffu);
}

#endif
