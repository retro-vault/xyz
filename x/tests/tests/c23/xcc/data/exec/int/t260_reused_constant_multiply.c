unsigned dynamic_product(unsigned left, unsigned right)
{
    return left * right;
}

unsigned scaled_right(unsigned value) { return value * 0x1357u; }
unsigned scaled_left(unsigned value) { return 0x2d39u * value; }
unsigned short_scale(unsigned value) { return value * 257u; }

unsigned repeated_scale(unsigned seed, unsigned count)
{
    unsigned total = 0;
    for (unsigned i = 0; i < count; ++i) {
        seed = seed * 0x1357u + 0x47u;
        total ^= seed;
    }
    return total;
}

static __attribute__((noinline)) unsigned multiply_reference(unsigned left,
                                                            unsigned right)
{
    unsigned result = 0;
    while (right != 0) {
        if (right & 1u) result += left;
        left <<= 1;
        right >>= 1;
    }
    return result;
}

int main(void)
{
    unsigned value = 0;
    for (unsigned i = 0; i < 1024; ++i) {
        if (scaled_right(value) != multiply_reference(value, 0x1357u)) return 1;
        if (scaled_left(value) != multiply_reference(value, 0x2d39u)) return 2;
        if (short_scale(value) != multiply_reference(value, 257u)) return 3;
        if (dynamic_product(value, i) != multiply_reference(value, i)) return 4;
        value += 0x83u;
    }
    for (unsigned count = 0; count < 19; ++count) {
        unsigned seed = 0xfff1u;
        unsigned expected = 0;
        for (unsigned i = 0; i < count; ++i) {
            seed = multiply_reference(seed, 0x1357u) + 0x47u;
            expected ^= seed;
        }
        if (repeated_scale(0xfff1u, count) != expected) return 5;
    }
    return 0;
}
