#ifndef XYZ_PORTABLE_BENCH_H
#define XYZ_PORTABLE_BENCH_H

/*
 * Shared helpers for the generated portable cross-compiler benchmark corpus.
 *
 * The corpus intentionally sticks to a small C89-friendly subset so the same
 * programs can be built by xcc, SDCC, and z88dk's 80cc/sccz80 frontends.
 * Every benchmark is self-checking and returns 0 on success.
 */

typedef unsigned char pb_u8;
typedef unsigned int pb_u16;

static pb_u16
pb_step(pb_u16 state)
{
    return (pb_u16)(state * 25173u + 13849u);
}

static pb_u8
pb_rand8(pb_u16 *state, pb_u8 salt)
{
    pb_u16 s;

    s = pb_step((pb_u16)(*state + (pb_u16)salt + 1u));
    *state = s;
    s ^= (pb_u16)(s >> 7);
    s ^= (pb_u16)(s >> 3);
    return (pb_u8)s;
}

static void
pb_fill_bytes(pb_u8 *buf, pb_u8 n, pb_u16 seed, pb_u8 salt)
{
    pb_u8 i;
    pb_u16 state;

    state = seed;
    for (i = 0u; i < n; ++i)
        buf[i] = pb_rand8(&state, (pb_u8)(salt + i));
}

static pb_u16
pb_rotl5(pb_u16 value)
{
    return (pb_u16)((value << 5) | (value >> 11));
}

static pb_u16
pb_mix(pb_u16 acc, pb_u16 value)
{
    acc ^= (pb_u16)(value + 0x9e37u);
    acc = pb_rotl5(acc);
    acc += (pb_u16)(value ^ 0x7f4au);
    return acc;
}

static void
pb_swap_u8(pb_u8 *lhs, pb_u8 *rhs)
{
    pb_u8 tmp;

    tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

static pb_u16
pb_absdiff_u8(pb_u8 lhs, pb_u8 rhs)
{
    if (lhs > rhs)
        return (pb_u16)(lhs - rhs);
    return (pb_u16)(rhs - lhs);
}

#endif
