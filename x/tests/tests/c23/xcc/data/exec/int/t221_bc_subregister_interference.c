typedef unsigned char u8;
typedef unsigned int u16;

static u8 samples[64];

static u8
seed_byte(u8 salt)
{
    u16 value = 0;

    value ^= (u16)salt;
    value ^= (u16)(value << 3);
    value ^= (u16)(value >> 5);
    value += (u16)(0x31u + salt);
    return (u8)value;
}

static u16
mix16(u16 acc, u16 value)
{
    acc ^= (u16)(value + 0x9e37u);
    acc = (u16)((acc << 5) | (acc >> 11));
    acc += (u16)(value ^ 0x7f4au);
    return acc;
}

int
main(void)
{
    u16 acc;
    u16 mix = 0x789au;
    u8 i;
    u8 fill_index;
    u8 fill_value = seed_byte((u8)(0xb2u ^ 0x5au));

    for (fill_index = 0; fill_index < 64u; ++fill_index) {
        fill_value ^= (u8)(fill_value << 3);
        fill_value ^= (u8)(fill_value >> 5);
        fill_value += (u8)(0xb2u + fill_index + 17u);
        samples[fill_index] = (u8)(fill_value ^ fill_index);
    }

    for (i = 7u; i < 64u; ++i) {
        acc = (u16)samples[i];
        acc += (u16)(samples[(u8)(i - 1u)] << 1);
        acc += (u16)samples[(u8)(i - 2u)];
        acc -= (u16)samples[(u8)(i - 3u)];
        acc += (u16)(samples[(u8)(i - 5u)] << 1);
        acc += (u16)samples[(u8)(i - 5u)];
        acc -= (u16)(samples[(u8)(i - 6u)] << 1);
        acc += (u16)samples[(u8)(i - 7u)];
        mix = mix16(mix, (u16)(acc & 0xffu));
    }

    return mix == 13926u ? 0 : 1;
}
