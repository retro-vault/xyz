struct packed_fields {
    unsigned int low : 3;
    unsigned int high : 5;
    signed int signed_low : 4;
    unsigned int signed_sibling : 4;
};

struct crossing_fields {
    unsigned int prefix : 7;
    unsigned int crossing : 9;
};

static volatile struct packed_fields volatile_fields;

static int
check_packed_fields(void)
{
    struct packed_fields fields = { 0 };
    unsigned int low;
    unsigned int high;

    for (low = 0; low < 8; ++low) {
        for (high = 0; high < 32; ++high) {
            fields.low = low;
            fields.high = high;
            fields.signed_low = -3;
            fields.signed_sibling = 13;
            if (fields.low != low || fields.high != high ||
                fields.signed_low != -3 || fields.signed_sibling != 13)
                return 1;

            fields.low = (low + 5u) & 7u;
            if (fields.high != high || fields.signed_low != -3 ||
                fields.signed_sibling != 13)
                return 2;
        }
    }
    return 0;
}

static int
check_crossing_field(void)
{
    struct crossing_fields fields = { 0 };
    unsigned int value;

    fields.prefix = 85u;
    for (value = 0; value < 512u; ++value) {
        fields.crossing = value;
        if (fields.crossing != value || fields.prefix != 85u)
            return 1;
    }
    return 0;
}

static int
check_volatile_fields(void)
{
    volatile_fields.low = 6u;
    volatile_fields.high = 27u;
    volatile_fields.signed_low = -5;
    volatile_fields.signed_sibling = 9u;
    if (volatile_fields.low != 6u || volatile_fields.high != 27u ||
        volatile_fields.signed_low != -5 ||
        volatile_fields.signed_sibling != 9u)
        return 1;

    volatile_fields.high = 3u;
    if (volatile_fields.low != 6u || volatile_fields.high != 3u ||
        volatile_fields.signed_low != -5 ||
        volatile_fields.signed_sibling != 9u)
        return 2;
    return 0;
}

int
main(void)
{
    int result = check_packed_fields();
    if (result != 0)
        return result;
    result = check_crossing_field();
    if (result != 0)
        return 10 + result;
    result = check_volatile_fields();
    if (result != 0)
        return 20 + result;
    return 0;
}
