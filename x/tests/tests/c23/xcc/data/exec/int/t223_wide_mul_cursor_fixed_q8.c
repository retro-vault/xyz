typedef unsigned int u16;
typedef unsigned long u32;

static const u16 values[4] = { 4660u, 43981u, 257u, 65535u };
static const u32 products[4] = {
    172420ul, 1627297ul, 9509ul, 2424795ul
};

static __attribute__((noinline)) int
check_products(const u16 *cursor, u16 factor)
{
    u16 index;

    for (index = 0; index < 4u; ++index) {
        u32 product = (u32)*cursor++ * factor;
        if (product != products[index])
            return (int)index + 1;
    }
    return 0;
}

static u16
q8_scale_37(u16 value)
{
    return (u16)(((u32)value * 37u) >> 8);
}

static __attribute__((noinline)) u16
fixed_sum(const u16 *cursor)
{
    u16 index;
    u16 sum = 0;

    for (index = 0; index < 4u; ++index)
        sum = (u16)(sum + q8_scale_37(*cursor++));
    return sum;
}

int
main(void)
{
    int product_error = check_products(values, 37u);

    if (product_error != 0)
        return product_error;
    if (fixed_sum(values) != 16537u)
        return 10;
    return 0;
}
