/* Truncation may discard high bits, but must keep the value captured earlier. */
unsigned char trunc_source;
signed char trunc_signed_source;
volatile unsigned char trunc_observable;

unsigned char trunc_add(unsigned char next) {
    unsigned saved = (unsigned)trunc_source;
    trunc_source = next;
    return (unsigned char)(saved + 7u);
}

unsigned char trunc_sub(unsigned char next) {
    unsigned saved = (unsigned)trunc_source;
    trunc_source = next;
    return (unsigned char)(123u - saved);
}

unsigned char trunc_xor(unsigned char next) {
    unsigned saved = (unsigned)trunc_source;
    trunc_source = next;
    return (unsigned char)(saved ^ 173u);
}

unsigned char trunc_shift(unsigned char next) {
    unsigned saved = (unsigned)trunc_source;
    trunc_source = next;
    return (unsigned char)(saved << 3);
}

unsigned char trunc_signed(signed char next) {
    unsigned saved = (unsigned)trunc_signed_source;
    trunc_signed_source = next;
    return (unsigned char)(saved + 9u);
}

unsigned char trunc_loop(unsigned char rounds) {
    unsigned saved = (unsigned)trunc_source;
    unsigned char sum = 0;
    unsigned char i;
    for (i = 0; i < rounds; ++i) {
        sum = (unsigned char)(sum + (unsigned char)(saved + 3u));
        ++trunc_source;
    }
    return sum;
}

unsigned char trunc_volatile(unsigned char next) {
    unsigned saved = (unsigned)trunc_observable;
    trunc_observable = next;
    return (unsigned char)(saved + 11u);
}

unsigned char trunc_bitint(unsigned char input) {
    unsigned _BitInt(3) narrow = (unsigned _BitInt(3))input;
    unsigned wide = (unsigned)narrow;
    return (unsigned char)(wide + 5u);
}

int main(void) {
    unsigned value;
    for (value = 0; value != 256; ++value) {
        unsigned char byte = (unsigned char)value;
        unsigned char next = (unsigned char)(value + 31u);
        trunc_source = byte;
        if (trunc_add(next) != (unsigned char)(value + 7u) ||
            trunc_source != next) return 1;
        trunc_source = byte;
        if (trunc_sub(next) != (unsigned char)(123u - value) ||
            trunc_source != next) return 2;
        trunc_source = byte;
        if (trunc_xor(next) != (unsigned char)(value ^ 173u) ||
            trunc_source != next) return 3;
        trunc_source = byte;
        if (trunc_shift(next) != (unsigned char)(value << 3) ||
            trunc_source != next) return 4;
        trunc_source = byte;
        if (trunc_loop(4) != (unsigned char)((value + 3u) * 4u) ||
            trunc_source != (unsigned char)(value + 4u)) return 5;
        trunc_observable = byte;
        if (trunc_volatile(next) != (unsigned char)(value + 11u) ||
            trunc_observable != next) return 6;
        if (trunc_bitint(byte) != (unsigned char)((value & 7u) + 5u))
            return 7;
    }
    trunc_signed_source = -5;
    if (trunc_signed(23) != 4 || trunc_signed_source != 23) return 8;
    return 0;
}
