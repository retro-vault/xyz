typedef unsigned char u8;
typedef unsigned int u16;
typedef signed int i16;
#define NOINLINE __attribute__((noinline))

NOINLINE u16 word_left_0(u16 value) { return value << 0; }
NOINLINE u16 word_right_0(u16 value) { return value >> 0; }
NOINLINE i16 word_signed_0(i16 value) { return value >> 0; }
NOINLINE u16 word_left_1(u16 value) { return value << 1; }
NOINLINE u16 word_right_1(u16 value) { return value >> 1; }
NOINLINE i16 word_signed_1(i16 value) { return value >> 1; }
NOINLINE u16 word_left_2(u16 value) { return value << 2; }
NOINLINE u16 word_right_2(u16 value) { return value >> 2; }
NOINLINE i16 word_signed_2(i16 value) { return value >> 2; }
NOINLINE u16 word_left_3(u16 value) { return value << 3; }
NOINLINE u16 word_right_3(u16 value) { return value >> 3; }
NOINLINE i16 word_signed_3(i16 value) { return value >> 3; }
NOINLINE u16 word_left_4(u16 value) { return value << 4; }
NOINLINE u16 word_right_4(u16 value) { return value >> 4; }
NOINLINE i16 word_signed_4(i16 value) { return value >> 4; }
NOINLINE u16 word_left_5(u16 value) { return value << 5; }
NOINLINE u16 word_right_5(u16 value) { return value >> 5; }
NOINLINE i16 word_signed_5(i16 value) { return value >> 5; }
NOINLINE u16 word_left_6(u16 value) { return value << 6; }
NOINLINE u16 word_right_6(u16 value) { return value >> 6; }
NOINLINE i16 word_signed_6(i16 value) { return value >> 6; }
NOINLINE u16 word_left_7(u16 value) { return value << 7; }
NOINLINE u16 word_right_7(u16 value) { return value >> 7; }
NOINLINE i16 word_signed_7(i16 value) { return value >> 7; }
NOINLINE u16 word_left_8(u16 value) { return value << 8; }
NOINLINE u16 word_right_8(u16 value) { return value >> 8; }
NOINLINE i16 word_signed_8(i16 value) { return value >> 8; }
NOINLINE u16 word_left_9(u16 value) { return value << 9; }
NOINLINE u16 word_right_9(u16 value) { return value >> 9; }
NOINLINE i16 word_signed_9(i16 value) { return value >> 9; }
NOINLINE u16 word_left_10(u16 value) { return value << 10; }
NOINLINE u16 word_right_10(u16 value) { return value >> 10; }
NOINLINE i16 word_signed_10(i16 value) { return value >> 10; }
NOINLINE u16 word_left_11(u16 value) { return value << 11; }
NOINLINE u16 word_right_11(u16 value) { return value >> 11; }
NOINLINE i16 word_signed_11(i16 value) { return value >> 11; }
NOINLINE u16 word_left_12(u16 value) { return value << 12; }
NOINLINE u16 word_right_12(u16 value) { return value >> 12; }
NOINLINE i16 word_signed_12(i16 value) { return value >> 12; }
NOINLINE u16 word_left_13(u16 value) { return value << 13; }
NOINLINE u16 word_right_13(u16 value) { return value >> 13; }
NOINLINE i16 word_signed_13(i16 value) { return value >> 13; }
NOINLINE u16 word_left_14(u16 value) { return value << 14; }
NOINLINE u16 word_right_14(u16 value) { return value >> 14; }
NOINLINE i16 word_signed_14(i16 value) { return value >> 14; }
NOINLINE u16 word_left_15(u16 value) { return value << 15; }
NOINLINE u16 word_right_15(u16 value) { return value >> 15; }
NOINLINE i16 word_signed_15(i16 value) { return value >> 15; }

static u16 (*const left_functions[])(u16) = {word_left_0, word_left_1, word_left_2, word_left_3, word_left_4, word_left_5, word_left_6, word_left_7, word_left_8, word_left_9, word_left_10, word_left_11, word_left_12, word_left_13, word_left_14, word_left_15};
static u16 (*const right_functions[])(u16) = {word_right_0, word_right_1, word_right_2, word_right_3, word_right_4, word_right_5, word_right_6, word_right_7, word_right_8, word_right_9, word_right_10, word_right_11, word_right_12, word_right_13, word_right_14, word_right_15};
static i16 (*const signed_functions[])(i16) = {word_signed_0, word_signed_1, word_signed_2, word_signed_3, word_signed_4, word_signed_5, word_signed_6, word_signed_7, word_signed_8, word_signed_9, word_signed_10, word_signed_11, word_signed_12, word_signed_13, word_signed_14, word_signed_15};
static const u16 values[] = {0u,1u,2u,127u,128u,255u,256u,257u,0x7fffu,0x8000u,0xffffu,0x55aau,0xaa55u,0x1234u,0x80ffu};

int main(void)
{
    u16 i;
    u16 j;
    for (i = 0; i < 16u; ++i)
        for (j = 0; j < sizeof(values) / sizeof(values[0]); ++j) {
            volatile u8 count = (u8)i;
            if (left_functions[i](values[j]) != (u16)(values[j] << count))
                return 1;
            if (right_functions[i](values[j]) != (u16)(values[j] >> count))
                return 2;
            if (signed_functions[i]((i16)values[j]) != (i16)((i16)values[j] >> count))
                return 3;
        }
    return 0;
}
