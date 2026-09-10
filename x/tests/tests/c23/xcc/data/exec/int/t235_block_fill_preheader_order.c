typedef unsigned char u8;
static u8 global_buffer[9];

static __attribute__((noinline)) void prepare(u8 *p) { p[0] = 3; }

static __attribute__((noinline)) void fill_after_store(u8 *p)
{
    unsigned i = 0;
    p[0] = 3;
    for (; i < 7; ++i) p[i] = 5;
}

static __attribute__((noinline)) void fill_after_call(u8 *p)
{
    unsigned i = 0;
    prepare(p);
    for (; i < 7; ++i) p[i] = 5;
}

static __attribute__((noinline)) void fill_computed_after_call(
    u8 *base, unsigned offset)
{
    u8 *p = base + offset;
    unsigned i = 0;
    prepare(base);
    for (; i < 7; ++i) p[i] = 5;
}

static __attribute__((noinline)) u8 read_before_fill(u8 *p)
{
    unsigned i = 0;
    u8 original = p[0];
    for (; i < 7; ++i) p[i] = 5;
    return original;
}

static __attribute__((noinline)) void fill_global_after_store(void)
{
    unsigned i = 0;
    global_buffer[0] = 3;
    for (; i < 7; ++i) global_buffer[i] = 5;
}

int main(void)
{
    u8 data[9];
    data[0] = 29; data[8] = 31;
    fill_after_store(data + 1);
    if (data[1] != 5 || data[7] != 5) return 1;
    fill_after_call(data + 1);
    if (data[1] != 5 || data[7] != 5) return 2;
    data[1] = 37;
    if (read_before_fill(data + 1) != 37 || data[1] != 5) return 3;
    if (data[0] != 29 || data[8] != 31) return 4;
    fill_computed_after_call(data, 1);
    if (data[0] != 3 || data[1] != 5 || data[7] != 5 || data[8] != 31)
        return 6;
    global_buffer[7] = 41;
    fill_global_after_store();
    if (global_buffer[0] != 5 || global_buffer[6] != 5 ||
        global_buffer[7] != 41) return 5;
    return 0;
}
