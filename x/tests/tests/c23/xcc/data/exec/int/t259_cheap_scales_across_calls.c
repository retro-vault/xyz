typedef unsigned (*operation)(unsigned);

unsigned dispatch_words(const operation *operations, const unsigned *values,
                        unsigned count)
{
    unsigned total = 0;
    for (unsigned index = 0; index < count; ++index)
        total += operations[index](values[index]);
    return total;
}

unsigned collect_pairs(unsigned (*visit)(unsigned, unsigned),
                       const unsigned (*pairs)[2], unsigned count)
{
    unsigned total = 0;
    for (unsigned index = 0; index < count; ++index)
        total += visit(pairs[index][0], pairs[index][1]);
    return total;
}

unsigned dispatch_reverse(const operation *operations, const unsigned *values,
                          unsigned count)
{
    unsigned total = 0;
    for (unsigned index = count; index != 0;) {
        --index;
        total += operations[index](values[index]);
    }
    return total;
}

#ifndef SCALE_CORE_ONLY
static unsigned bump(unsigned value) { return value + 0x127u; }
static unsigned flip(unsigned value) { return value ^ 0x5a6bu; }
static unsigned rotate(unsigned value)
{
    return (value << 3) | (value >> 13);
}
static unsigned combine(unsigned left, unsigned right)
{
    return left * 3u + right * 5u;
}

static operation operations[257];
static unsigned values[257];
static unsigned pairs[257][2];

int main(void)
{
    static const unsigned counts[] = {0, 1, 2, 3, 7, 31, 63, 127, 255, 256, 257};
    static const unsigned dispatch_expected[] = {
        0, 298, 23465, 23761, 48298, 63562, 3685, 29898, 46757, 51390, 5158
    };
    static const unsigned pair_expected[] = {
        0, 44, 304, 780, 4844, 36268, 31404, 29868, 59564, 49152, 38956
    };
    for (unsigned i = 0; i < 257; ++i) {
        values[i] = i * 17u + 3u;
        pairs[i][0] = values[i];
        pairs[i][1] = i * 33u + 7u;
        operations[i] = i % 3u == 0 ? bump : (i % 3u == 1 ? flip : rotate);
    }
    for (unsigned i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
        unsigned count = counts[i];
        if (dispatch_words(operations, values, count) != dispatch_expected[i])
            return 1;
        if (dispatch_reverse(operations, values, count) != dispatch_expected[i])
            return 2;
        if (collect_pairs(combine, pairs, count) != pair_expected[i])
            return 3;
    }
    return 0;
}
#endif
