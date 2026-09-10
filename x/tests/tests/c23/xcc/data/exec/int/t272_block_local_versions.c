#define NOINLINE __attribute__((noinline))
volatile unsigned observed;

NOINLINE unsigned version_word(unsigned seed, unsigned *destination) {
    unsigned state = seed * 29u + 123u;
    *destination = state;
    state ^= state >> 5;
    state ^= state >> 9;
    return state;
}
NOINLINE unsigned version_byte(unsigned seed) {
    unsigned char state = (unsigned char)seed;
    state ^= state >> 3;
    state += 17;
    state ^= state << 2;
    return state;
}
NOINLINE unsigned reference_word(unsigned seed) {
    unsigned initial = seed * 29u + 123u;
    unsigned first = initial ^ (initial >> 5);
    return first ^ (first >> 9);
}
NOINLINE unsigned reference_byte(unsigned seed) {
    unsigned char initial = (unsigned char)seed;
    unsigned char first = initial ^ (initial >> 3);
    unsigned char second = first + 17;
    return (unsigned char)(second ^ (second << 2));
}
NOINLINE void modify_alias(unsigned *value) { *value ^= 0x5a31u; }
NOINLINE unsigned escaped_word(unsigned seed) {
    unsigned state = seed + 3;
    modify_alias(&state);
    state ^= state >> 5;
    return state;
}
NOINLINE unsigned partial_word(unsigned seed) {
    unsigned state = seed;
    ((unsigned char *)&state)[1] ^= 0x5a;
    state ^= state >> 5;
    return state;
}
NOINLINE unsigned branch_word(unsigned seed, unsigned condition) {
    unsigned state = seed;
    if (condition) state ^= state >> 5;
    else state += 71;
    state ^= state >> 9;
    return state;
}
NOINLINE unsigned loop_word(unsigned seed, unsigned count) {
    unsigned state = seed;
    do { state ^= state >> 5; state += 71; } while (--count);
    return state;
}
NOINLINE unsigned block_each_visit(unsigned seed, unsigned count) {
    unsigned sum = 0;
    do {
        unsigned state = seed + count;
        state ^= state >> 5;
        state += 71;
        sum += state;
    } while (--count);
    return sum;
}
NOINLINE unsigned capture_volatile(unsigned seed) {
    unsigned state = observed;
    observed = seed;
    state ^= state >> 5;
    state ^= state >> 9;
    return state;
}
NOINLINE unsigned volatile_local(unsigned seed) {
    volatile unsigned state = seed;
    state ^= state >> 5;
    state += 71;
    return state;
}
int main(void) {
    unsigned index, destination;
    for (index = 0; index < 256; ++index) {
        unsigned seed = index * 257u, initial, first, expected, count;
        if (version_word(seed, &destination) != reference_word(seed) ||
            destination != (unsigned)(seed * 29u + 123u)) return 1;
        if (version_byte(seed) != reference_byte(seed)) return 2;
        initial = (seed + 3) ^ 0x5a31u;
        if (escaped_word(seed) != (initial ^ (initial >> 5))) return 3;
        initial = seed ^ 0x5a00u;
        if (partial_word(seed) != (initial ^ (initial >> 5))) return 4;
        first = seed ^ (seed >> 5);
        if (branch_word(seed, 1) != (first ^ (first >> 9))) return 5;
        first = seed + 71;
        if (branch_word(seed, 0) != (first ^ (first >> 9))) return 6;
        expected = seed;
        for (count = 0; count < 3; ++count) {
            expected ^= expected >> 5;
            expected += 71;
        }
        if (loop_word(seed, 3) != expected) return 7;
        expected = 0;
        for (count = 1; count <= 3; ++count) {
            initial = seed + count;
            expected += (unsigned)((initial ^ (initial >> 5)) + 71);
        }
        if (block_each_visit(seed, 3) != expected) return 8;
        observed = seed;
        first = seed ^ (seed >> 5);
        if (capture_volatile(seed + 19) != (first ^ (first >> 9)) ||
            observed != (unsigned)(seed + 19)) return 9;
        if (volatile_local(seed) != (unsigned)((seed ^ (seed >> 5)) + 71)) return 10;
    }
    return 0;
}
