#ifndef TEST_CASE
#define TEST_CASE 0
#endif

typedef unsigned char u8;
typedef unsigned int u16;

#if TEST_CASE == 0 || TEST_CASE == 1
__attribute__((noinline)) u8 cached_byte_walk(u8 *cursor)
{
    u8 value = 0x69;
    u8 *end = cursor + 19;
    while (cursor < end) {
        value ^= *cursor;
        ++cursor;
        value = (value & 0x80u)
            ? ((u8)(value << 1) ^ 0x53u) : (u8)(value << 1);
        value = (value & 0x80u)
            ? ((u8)(value << 1) ^ 0xabu) : (u8)(value << 1);
    }
    return value;
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 2
__attribute__((noinline)) u8 observable_byte_step(u8 input)
{
    volatile u8 value = input;
    u8 snapshot = value;
    return (snapshot & 0x80u)
        ? ((u8)(snapshot << 1) ^ 0x53u) : (u8)(snapshot << 1);
}
#endif

#if TEST_CASE == 0 || TEST_CASE == 3
__attribute__((noinline)) u8 observable_direct_byte_step(u8 input)
{
    volatile u8 value = input;
    return (value & 0x80u)
        ? ((u8)(value << 1) ^ 0x53u) : (u8)(value << 1);
}
#endif

#if TEST_CASE == 0
__attribute__((noinline)) u16 changed_pointer_bytes(u8 *cursor, u16 count)
{
    u16 sum = 0;
    while (count--) {
        sum += (u8)(u16)cursor;
        ++cursor;
        sum += (u8)(u16)cursor;
    }
    return sum;
}

__attribute__((noinline)) u16 changed_pointer_high_bytes(u8 *cursor, u16 count)
{
    u16 sum = 0;
    while (count--) {
        sum += (u8)((u16)cursor >> 8);
        ++cursor;
        sum += (u8)((u16)cursor >> 8);
    }
    return sum;
}

static u8 reference_step(u8 value, u8 polynomial)
{
    u8 mask = (u8)(0u - (value >> 7));
    return (u8)((value << 1) ^ (mask & polynomial));
}

int main(void)
{
    u8 data[300];
    u16 input, i;
    for (i = 0; i < 23; ++i) data[i] = (u8)(i * 37u + 91u);
    for (input = 0; input < 256; ++input) {
        u16 count = input % 23u + 1u;
        u8 expected = 0x69;
        u16 pointer_sum = 0;
        u16 pointer_high_sum = 0;
        u16 base = (u16)(data + input);
        data[0] = (u8)input;
        for (i = 0; i < 19; ++i) {
            expected ^= data[i];
            expected = reference_step(expected, 0x53u);
            expected = reference_step(expected, 0xabu);
        }
        for (i = 0; i < count; ++i) {
            pointer_sum += (u8)(base + i);
            pointer_sum += (u8)(base + i + 1u);
            pointer_high_sum += (u8)((base + i) >> 8);
            pointer_high_sum += (u8)((base + i + 1u) >> 8);
        }
        if (cached_byte_walk(data) != expected) return 1;
        if (observable_byte_step((u8)input) != reference_step((u8)input, 0x53u))
            return 2;
        if (observable_direct_byte_step((u8)input) != reference_step((u8)input, 0x53u))
            return 5;
        if (changed_pointer_bytes(data + input, count) != pointer_sum) return 3;
        if (changed_pointer_high_bytes(data + input, count) != pointer_high_sum)
            return 4;
    }
    return 0;
}
#endif
