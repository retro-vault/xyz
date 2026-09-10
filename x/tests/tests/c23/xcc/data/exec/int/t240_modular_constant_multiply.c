// Exercise modular multiplication independently of the synthesized chains.
typedef unsigned char u8;
typedef unsigned int u16;
#define NOINLINE __attribute__((noinline))

static NOINLINE u16 reference(u16 value, u16 factor)
{
    volatile u16 dynamic_factor = factor;
    return value * dynamic_factor;
}

static NOINLINE u16 word_3(u16 value) { return value * 3u; }
static NOINLINE u16 word_5(u16 value) { return value * 5u; }
static NOINLINE u16 word_7(u16 value) { return value * 7u; }
static NOINLINE u16 word_11(u16 value) { return value * 11u; }
static NOINLINE u16 word_15(u16 value) { return value * 15u; }
static NOINLINE u16 word_31(u16 value) { return value * 31u; }
static NOINLINE u16 word_37(u16 value) { return value * 37u; }
static NOINLINE u16 word_63(u16 value) { return value * 63u; }
static NOINLINE u16 word_127(u16 value) { return value * 127u; }
static NOINLINE u16 word_129(u16 value) { return value * 129u; }
static NOINLINE u16 word_255(u16 value) { return value * 255u; }
static NOINLINE u16 word_257(u16 value) { return value * 257u; }
static NOINLINE u16 word_511(u16 value) { return value * 511u; }
static NOINLINE u16 word_513(u16 value) { return value * 513u; }
static NOINLINE u16 word_1023(u16 value) { return value * 1023u; }
static NOINLINE u16 word_1025(u16 value) { return value * 1025u; }
static NOINLINE u16 word_4095(u16 value) { return value * 4095u; }
static NOINLINE u16 word_4097(u16 value) { return value * 4097u; }
static NOINLINE u16 word_16383(u16 value) { return value * 16383u; }
static NOINLINE u16 word_32767(u16 value) { return value * 32767u; }
static NOINLINE u16 word_32769(u16 value) { return value * 32769u; }
static NOINLINE u16 word_65279(u16 value) { return value * 65279u; }
static NOINLINE u16 word_65281(u16 value) { return value * 65281u; }
static NOINLINE u16 word_65533(u16 value) { return value * 65533u; }
static NOINLINE u16 word_65535(u16 value) { return value * 65535u; }
static NOINLINE u8 byte_3(u8 value) { return (u8)(value * 3u); }
static NOINLINE u8 byte_7(u8 value) { return (u8)(value * 7u); }
static NOINLINE u8 byte_15(u8 value) { return (u8)(value * 15u); }
static NOINLINE u8 byte_31(u8 value) { return (u8)(value * 31u); }
static NOINLINE u8 byte_63(u8 value) { return (u8)(value * 63u); }
static NOINLINE u8 byte_127(u8 value) { return (u8)(value * 127u); }
static NOINLINE u8 byte_129(u8 value) { return (u8)(value * 129u); }
static NOINLINE u8 byte_191(u8 value) { return (u8)(value * 191u); }
static NOINLINE u8 byte_223(u8 value) { return (u8)(value * 223u); }
static NOINLINE u8 byte_251(u8 value) { return (u8)(value * 251u); }
static NOINLINE u8 byte_253(u8 value) { return (u8)(value * 253u); }
static NOINLINE u8 byte_255(u8 value) { return (u8)(value * 255u); }

static const u16 factors[] = { 3u, 5u, 7u, 11u, 15u, 31u, 37u, 63u, 127u, 129u, 255u, 257u, 511u, 513u, 1023u, 1025u, 4095u, 4097u, 16383u, 32767u, 32769u, 65279u, 65281u, 65533u, 65535u };
static u16 (*const words[])(u16) = { word_3, word_5, word_7, word_11, word_15, word_31, word_37, word_63, word_127, word_129, word_255, word_257, word_511, word_513, word_1023, word_1025, word_4095, word_4097, word_16383, word_32767, word_32769, word_65279, word_65281, word_65533, word_65535 };
static const u8 byte_factors[] = { 3, 7, 15, 31, 63, 127, 129, 191, 223, 251, 253, 255 };
static u8 (*const byte_functions[])(u8) = { byte_3, byte_7, byte_15, byte_31, byte_63, byte_127, byte_129, byte_191, byte_223, byte_251, byte_253, byte_255 };
static const u16 edges[] = {0u,1u,2u,127u,128u,255u,256u,257u,32767u,32768u,32769u,65280u,65534u,65535u};

int main(void)
{
    u16 i;
    u16 j;
    u16 state = 0x1234u;
    for (i = 0; i < sizeof(factors) / sizeof(factors[0]); ++i) {
        for (j = 0; j < sizeof(edges) / sizeof(edges[0]); ++j)
            if (words[i](edges[j]) != reference(edges[j], factors[i]))
                return 1;
        for (j = 0; j < 32u; ++j) {
            state = (state << 1) ^ ((state & 0x8000u) ? 0x1021u : 0u);
            if (words[i](state) != reference(state, factors[i]))
                return 2;
        }
    }
    for (i = 0; i < sizeof(byte_factors); ++i)
        for (j = 0; j < 256u; ++j)
            if (byte_functions[i]((u8)j) != (u8)reference(j, byte_factors[i]))
                return 3;
    return 0;
}
