typedef unsigned _BitInt(8) u8;
typedef _BitInt(8) s8;
unsigned shift_left(unsigned packed) {
    u8 value = (u8)packed;
    unsigned count = packed >> 8;
    return (unsigned)(value << count);
}
unsigned shift_logical(unsigned packed) {
    u8 value = (u8)packed;
    unsigned count = packed >> 8;
    return (unsigned)(value >> count);
}
unsigned shift_arithmetic(unsigned packed) {
    s8 value = (s8)packed;
    unsigned count = packed >> 8;
    return (unsigned char)(value >> count);
}
