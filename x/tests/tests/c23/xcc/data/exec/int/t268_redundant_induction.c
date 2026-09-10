#define NOINLINE __attribute__((noinline))
unsigned data[1024];
unsigned other[173];

#define WORD_CASE(name, start, stride, count) \
    NOINLINE unsigned name(void) { \
        unsigned sum = 0, i, j = start; \
        for (i = 0; i < count; ++i, j += stride) sum += data[j & 1023u] + (j >> 8); \
        return sum; \
    }
WORD_CASE(stride_two, 5, 2, 173)
WORD_CASE(stride_seven, 3, 7, 131)
WORD_CASE(stride_thirteen, 11, 13, 257)
WORD_CASE(last_word_value, 65520u, 3, 5)
WORD_CASE(wrapping_word, 65520u, 3, 9)
WORD_CASE(empty_count, 7, 5, 0)
WORD_CASE(single_count, 7, 5, 1)

#ifndef INDUCTION_CORE_ONLY
NOINLINE unsigned reference(unsigned start, unsigned stride, unsigned count) {
    unsigned sum = 0;
    while (count != 0) {
        --count;
        sum += data[start & 1023u] + (start >> 8);
        start += stride;
    }
    return sum;
}
NOINLINE unsigned observed_counter(unsigned limit) {
    unsigned i, j = 5, sum = 0;
    for (i = 0; i < 173; ++i, j += 2) {
        if (i == limit) break;
        sum += data[j & 1023u] + (j >> 8);
    }
    return sum ^ i;
}
NOINLINE unsigned early_exit(unsigned stop) {
    unsigned i, j = 5, sum = 0;
    for (i = 0; i < 173; ++i, j += 2) {
        if (j == stop) break;
        sum += data[j & 1023u] + (j >> 8);
    }
    return sum;
}
NOINLINE unsigned wrapping_byte(void) {
    unsigned i, sum = 0;
    unsigned char j = 247;
    for (i = 0; i < 257; ++i, j += 7) sum += data[j];
    return sum;
}
NOINLINE unsigned inclusive_step(void) {
    unsigned i, j = 4, sum = 0;
    for (i = 2; i <= 32; i += 3, j += 7) sum += data[j];
    return sum;
}
NOINLINE unsigned signed_step(void) {
    int i, j = 13;
    unsigned sum = 0;
    for (i = 2; i < 33; i += 3, j += 7) sum += data[j];
    return sum;
}
NOINLINE unsigned continue_step(void) {
    unsigned i, j = 5, sum = 0;
    for (i = 0; i < 173; ++i, j += 2) {
        if ((j & 7u) == 3) continue;
        sum += data[j & 1023u] + (j >> 8);
    }
    return sum;
}
NOINLINE unsigned volatile_control(void) {
    volatile unsigned i;
    unsigned j = 5, sum = 0;
    for (i = 0; i < 17; ++i, j += 2) sum += data[j];
    return sum;
}
static volatile unsigned observations;
NOINLINE unsigned observe(unsigned index) {
    ++observations;
    return data[index];
}
NOINLINE unsigned calls_in_body(void) {
    unsigned i, j = 5, sum = 0;
    for (i = 0; i < 17; ++i, j += 2) sum += observe(j);
    return sum;
}
NOINLINE unsigned paired_words(void) {
    unsigned i, sum = 0;
    for (i = 0; i < 173; ++i) sum += data[i] + other[i];
    return sum;
}
int main(void) {
    unsigned i;
    for (i = 0; i < 1024; ++i) data[i] = i * 73u + 0x1357u;
    for (i = 0; i < 173; ++i) other[i] = i * 37u + 0x2468u;
    if (stride_two() != reference(5, 2, 173)) return 1;
    if (stride_seven() != reference(3, 7, 131)) return 2;
    if (stride_thirteen() != reference(11, 13, 257)) return 3;
    if (last_word_value() != reference(65520u, 3, 5)) return 4;
    if (wrapping_word() != reference(65520u, 3, 9)) return 5;
    if (empty_count() != 0) return 6;
    if (single_count() != reference(7, 5, 1)) return 7;
    for (i = 0; i <= 173; i += 17) {
        if (early_exit(5 + 2 * i) != reference(5, 2, i)) return 8;
        if (observed_counter(i) != (reference(5, 2, i) ^ i)) return 9;
    }
    if (early_exit(0) != reference(5, 2, 173)) return 10;
    {
        unsigned expected = 0;
        unsigned char index = 247;
        for (i = 0; i < 257; ++i) {
            expected += data[index];
            index = (unsigned char)(index + 7);
        }
        if (wrapping_byte() != expected) return 11;
    }
    if (inclusive_step() != reference(4, 7, 11)) return 12;
    if (volatile_control() != reference(5, 2, 17)) return 13;
    observations = 0;
    if (calls_in_body() != reference(5, 2, 17) || observations != 17) return 14;
    if (signed_step() != reference(13, 7, 11)) return 15;
    {
        unsigned expected = 0, index = 5;
        for (i = 0; i < 173; ++i) {
            if ((index & 7u) != 3)
                expected += data[index & 1023u] + (index >> 8);
            index += 2;
        }
        if (continue_step() != expected) return 16;
    }
    {
        unsigned expected = 0;
        for (i = 173; i != 0; --i) expected += data[i - 1] + other[i - 1];
        if (paired_words() != expected) return 17;
    }
    return 0;
}
#endif
