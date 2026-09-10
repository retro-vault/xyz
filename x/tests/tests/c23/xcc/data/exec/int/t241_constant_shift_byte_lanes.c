// Compare immediate shift lowering with the independent variable-count path.
typedef unsigned char u8;
typedef signed char i8;
typedef unsigned long u32;
typedef signed long i32;
#define NOINLINE __attribute__((noinline))

static NOINLINE u32 left_reference(u32 value, u8 count)
{ volatile u8 n = count; return value << n; }
static NOINLINE u32 right_reference(u32 value, u8 count)
{ volatile u8 n = count; return value >> n; }
static NOINLINE i32 signed_reference(i32 value, u8 count)
{ volatile u8 n = count; return value >> n; }
static NOINLINE u32 left_1(u32 value) { return value << 1; }
static NOINLINE u32 right_1(u32 value) { return value >> 1; }
static NOINLINE i32 signed_1(i32 value) { return value >> 1; }
static NOINLINE u32 left_2(u32 value) { return value << 2; }
static NOINLINE u32 right_2(u32 value) { return value >> 2; }
static NOINLINE i32 signed_2(i32 value) { return value >> 2; }
static NOINLINE u32 left_3(u32 value) { return value << 3; }
static NOINLINE u32 right_3(u32 value) { return value >> 3; }
static NOINLINE i32 signed_3(i32 value) { return value >> 3; }
static NOINLINE u32 left_4(u32 value) { return value << 4; }
static NOINLINE u32 right_4(u32 value) { return value >> 4; }
static NOINLINE i32 signed_4(i32 value) { return value >> 4; }
static NOINLINE u32 left_5(u32 value) { return value << 5; }
static NOINLINE u32 right_5(u32 value) { return value >> 5; }
static NOINLINE i32 signed_5(i32 value) { return value >> 5; }
static NOINLINE u32 left_6(u32 value) { return value << 6; }
static NOINLINE u32 right_6(u32 value) { return value >> 6; }
static NOINLINE i32 signed_6(i32 value) { return value >> 6; }
static NOINLINE u32 left_7(u32 value) { return value << 7; }
static NOINLINE u32 right_7(u32 value) { return value >> 7; }
static NOINLINE i32 signed_7(i32 value) { return value >> 7; }
static NOINLINE u32 left_8(u32 value) { return value << 8; }
static NOINLINE u32 right_8(u32 value) { return value >> 8; }
static NOINLINE i32 signed_8(i32 value) { return value >> 8; }
static NOINLINE u32 left_9(u32 value) { return value << 9; }
static NOINLINE u32 right_9(u32 value) { return value >> 9; }
static NOINLINE i32 signed_9(i32 value) { return value >> 9; }
static NOINLINE u32 left_10(u32 value) { return value << 10; }
static NOINLINE u32 right_10(u32 value) { return value >> 10; }
static NOINLINE i32 signed_10(i32 value) { return value >> 10; }
static NOINLINE u32 left_11(u32 value) { return value << 11; }
static NOINLINE u32 right_11(u32 value) { return value >> 11; }
static NOINLINE i32 signed_11(i32 value) { return value >> 11; }
static NOINLINE u32 left_12(u32 value) { return value << 12; }
static NOINLINE u32 right_12(u32 value) { return value >> 12; }
static NOINLINE i32 signed_12(i32 value) { return value >> 12; }
static NOINLINE u32 left_13(u32 value) { return value << 13; }
static NOINLINE u32 right_13(u32 value) { return value >> 13; }
static NOINLINE i32 signed_13(i32 value) { return value >> 13; }
static NOINLINE u32 left_14(u32 value) { return value << 14; }
static NOINLINE u32 right_14(u32 value) { return value >> 14; }
static NOINLINE i32 signed_14(i32 value) { return value >> 14; }
static NOINLINE u32 left_15(u32 value) { return value << 15; }
static NOINLINE u32 right_15(u32 value) { return value >> 15; }
static NOINLINE i32 signed_15(i32 value) { return value >> 15; }
static NOINLINE u32 left_16(u32 value) { return value << 16; }
static NOINLINE u32 right_16(u32 value) { return value >> 16; }
static NOINLINE i32 signed_16(i32 value) { return value >> 16; }
static NOINLINE u32 left_17(u32 value) { return value << 17; }
static NOINLINE u32 right_17(u32 value) { return value >> 17; }
static NOINLINE i32 signed_17(i32 value) { return value >> 17; }
static NOINLINE u32 left_18(u32 value) { return value << 18; }
static NOINLINE u32 right_18(u32 value) { return value >> 18; }
static NOINLINE i32 signed_18(i32 value) { return value >> 18; }
static NOINLINE u32 left_19(u32 value) { return value << 19; }
static NOINLINE u32 right_19(u32 value) { return value >> 19; }
static NOINLINE i32 signed_19(i32 value) { return value >> 19; }
static NOINLINE u32 left_20(u32 value) { return value << 20; }
static NOINLINE u32 right_20(u32 value) { return value >> 20; }
static NOINLINE i32 signed_20(i32 value) { return value >> 20; }
static NOINLINE u32 left_21(u32 value) { return value << 21; }
static NOINLINE u32 right_21(u32 value) { return value >> 21; }
static NOINLINE i32 signed_21(i32 value) { return value >> 21; }
static NOINLINE u32 left_22(u32 value) { return value << 22; }
static NOINLINE u32 right_22(u32 value) { return value >> 22; }
static NOINLINE i32 signed_22(i32 value) { return value >> 22; }
static NOINLINE u32 left_23(u32 value) { return value << 23; }
static NOINLINE u32 right_23(u32 value) { return value >> 23; }
static NOINLINE i32 signed_23(i32 value) { return value >> 23; }
static NOINLINE u32 left_24(u32 value) { return value << 24; }
static NOINLINE u32 right_24(u32 value) { return value >> 24; }
static NOINLINE i32 signed_24(i32 value) { return value >> 24; }
static NOINLINE u32 left_25(u32 value) { return value << 25; }
static NOINLINE u32 right_25(u32 value) { return value >> 25; }
static NOINLINE i32 signed_25(i32 value) { return value >> 25; }
static NOINLINE u32 left_26(u32 value) { return value << 26; }
static NOINLINE u32 right_26(u32 value) { return value >> 26; }
static NOINLINE i32 signed_26(i32 value) { return value >> 26; }
static NOINLINE u32 left_27(u32 value) { return value << 27; }
static NOINLINE u32 right_27(u32 value) { return value >> 27; }
static NOINLINE i32 signed_27(i32 value) { return value >> 27; }
static NOINLINE u32 left_28(u32 value) { return value << 28; }
static NOINLINE u32 right_28(u32 value) { return value >> 28; }
static NOINLINE i32 signed_28(i32 value) { return value >> 28; }
static NOINLINE u32 left_29(u32 value) { return value << 29; }
static NOINLINE u32 right_29(u32 value) { return value >> 29; }
static NOINLINE i32 signed_29(i32 value) { return value >> 29; }
static NOINLINE u32 left_30(u32 value) { return value << 30; }
static NOINLINE u32 right_30(u32 value) { return value >> 30; }
static NOINLINE i32 signed_30(i32 value) { return value >> 30; }
static NOINLINE u32 left_31(u32 value) { return value << 31; }
static NOINLINE u32 right_31(u32 value) { return value >> 31; }
static NOINLINE i32 signed_31(i32 value) { return value >> 31; }
static NOINLINE u8 byte_left_1(u8 value) { return (u8)(value << 1); }
static NOINLINE u8 byte_right_1(u8 value) { return (u8)(value >> 1); }
static NOINLINE i8 byte_signed_1(i8 value) { return (i8)(value >> 1); }
static NOINLINE u8 byte_left_2(u8 value) { return (u8)(value << 2); }
static NOINLINE u8 byte_right_2(u8 value) { return (u8)(value >> 2); }
static NOINLINE i8 byte_signed_2(i8 value) { return (i8)(value >> 2); }
static NOINLINE u8 byte_left_3(u8 value) { return (u8)(value << 3); }
static NOINLINE u8 byte_right_3(u8 value) { return (u8)(value >> 3); }
static NOINLINE i8 byte_signed_3(i8 value) { return (i8)(value >> 3); }
static NOINLINE u8 byte_left_4(u8 value) { return (u8)(value << 4); }
static NOINLINE u8 byte_right_4(u8 value) { return (u8)(value >> 4); }
static NOINLINE i8 byte_signed_4(i8 value) { return (i8)(value >> 4); }
static NOINLINE u8 byte_left_5(u8 value) { return (u8)(value << 5); }
static NOINLINE u8 byte_right_5(u8 value) { return (u8)(value >> 5); }
static NOINLINE i8 byte_signed_5(i8 value) { return (i8)(value >> 5); }
static NOINLINE u8 byte_left_6(u8 value) { return (u8)(value << 6); }
static NOINLINE u8 byte_right_6(u8 value) { return (u8)(value >> 6); }
static NOINLINE i8 byte_signed_6(i8 value) { return (i8)(value >> 6); }
static NOINLINE u8 byte_left_7(u8 value) { return (u8)(value << 7); }
static NOINLINE u8 byte_right_7(u8 value) { return (u8)(value >> 7); }
static NOINLINE i8 byte_signed_7(i8 value) { return (i8)(value >> 7); }

static u32 (*const left_functions[])(u32) = {left_1, left_2, left_3, left_4, left_5, left_6, left_7, left_8, left_9, left_10, left_11, left_12, left_13, left_14, left_15, left_16, left_17, left_18, left_19, left_20, left_21, left_22, left_23, left_24, left_25, left_26, left_27, left_28, left_29, left_30, left_31};
static u32 (*const right_functions[])(u32) = {right_1, right_2, right_3, right_4, right_5, right_6, right_7, right_8, right_9, right_10, right_11, right_12, right_13, right_14, right_15, right_16, right_17, right_18, right_19, right_20, right_21, right_22, right_23, right_24, right_25, right_26, right_27, right_28, right_29, right_30, right_31};
static i32 (*const signed_functions[])(i32) = {signed_1, signed_2, signed_3, signed_4, signed_5, signed_6, signed_7, signed_8, signed_9, signed_10, signed_11, signed_12, signed_13, signed_14, signed_15, signed_16, signed_17, signed_18, signed_19, signed_20, signed_21, signed_22, signed_23, signed_24, signed_25, signed_26, signed_27, signed_28, signed_29, signed_30, signed_31};
static u8 (*const byte_left_functions[])(u8) = {byte_left_1, byte_left_2, byte_left_3, byte_left_4, byte_left_5, byte_left_6, byte_left_7};
static u8 (*const byte_right_functions[])(u8) = {byte_right_1, byte_right_2, byte_right_3, byte_right_4, byte_right_5, byte_right_6, byte_right_7};
static i8 (*const byte_signed_functions[])(i8) = {byte_signed_1, byte_signed_2, byte_signed_3, byte_signed_4, byte_signed_5, byte_signed_6, byte_signed_7};
static const u32 values[] = {0ul, 1ul, 0x12345678ul, 0x80ff017ful, 0x80000000ul, 0x7ffffffful, 0xfffffffful, 0x55aa55aaul, 0xaa55aa55ul};

int main(void)
{
    unsigned int i;
    unsigned int j;
    for (i = 0; i < 31u; ++i)
        for (j = 0; j < sizeof(values) / sizeof(values[0]); ++j) {
            if (left_functions[i](values[j]) != left_reference(values[j], (u8)(i + 1u)))
                return 1;
            if (right_functions[i](values[j]) != right_reference(values[j], (u8)(i + 1u)))
                return 2;
            if (signed_functions[i]((i32)values[j]) != signed_reference((i32)values[j], (u8)(i + 1u)))
                return 3;
        }
    for (i = 0; i < 7u; ++i)
        for (j = 0; j < 256u; ++j) {
            volatile u8 n = (u8)(i + 1u);
            if (byte_left_functions[i]((u8)j) != (u8)(j << n))
                return 4;
            if (byte_right_functions[i]((u8)j) != (u8)(j >> n))
                return 5;
            if (byte_signed_functions[i]((i8)j) != (i8)((i8)j >> n))
                return 6;
        }
    return 0;
}
