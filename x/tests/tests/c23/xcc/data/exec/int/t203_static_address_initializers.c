/* Regression: static pointer initializers must retain symbol relocations. */
static int object;
static int values[3];

struct record {
    unsigned char tag;
    int value;
};

static struct record item;
static int callback_count;

static void callback(void)
{
    ++callback_count;
}

static void *object_ptrs[] = {
    &object,
    &values[2],
    values,
    &item.value
};

static void (*function_ptrs[])(void) = {
    &callback,
    callback
};

static void *scalar_ptr = &object;

struct numeric_record {
    const unsigned char *seekadr;
    int txtlen;
};

/* Absolute pointer constants occur in memory maps and legacy file-offset
 * tables.  Exercise both the standard casted spelling and the traditional
 * implicit integer-to-pointer initializer accepted by XCC. */
static void *numeric_scalar_ptr = (void *)0x1234u;
static int *numeric_arithmetic_ptr = (int *)0x1200u + 3;
static const struct numeric_record numeric_records[] = {
    { (const unsigned char *)0x4567u, 52 },
    { 29548, 65 },
    { (const unsigned char *)0, 9 }
};

/* A qualified forward record declaration used to detach the array element
 * from the canonical tag.  Dynamic subscripts then advanced one byte instead
 * of sizeof(struct late_record). */
extern const struct late_record late_records[3];

struct late_record {
    const unsigned char *seekadr;
    int txtlen;
};

const struct late_record late_records[3] = {
    { (const unsigned char *)0x1111u, 11 },
    { (const unsigned char *)0x2222u, 22 },
    { (const unsigned char *)0x3333u, 33 }
};

static void **local_table(void)
{
    static void *entries[] = {
        &object,
        &values[1]
    };
    return entries;
}

int main(void)
{
    void **entries = local_table();

    if (object_ptrs[0] != &object)
        return 1;
    if (object_ptrs[1] != &values[2])
        return 2;
    if (object_ptrs[2] != values)
        return 3;
    if (object_ptrs[3] != &item.value)
        return 4;
    function_ptrs[0]();
    if (callback_count != 1)
        return 5;
    function_ptrs[1]();
    if (callback_count != 2)
        return 6;
    if (scalar_ptr != &object)
        return 7;
    if (entries[0] != &object)
        return 8;
    if (entries[1] != &values[1])
        return 9;
    if (numeric_scalar_ptr != (void *)0x1234u)
        return 10;
    if (numeric_arithmetic_ptr != (int *)0x1206u)
        return 11;
    if (numeric_records[0].seekadr != (const unsigned char *)0x4567u ||
        numeric_records[0].txtlen != 52)
        return 12;
    if (numeric_records[1].seekadr != (const unsigned char *)29548u ||
        numeric_records[1].txtlen != 65)
        return 13;
    if (numeric_records[2].seekadr != (const unsigned char *)0 ||
        numeric_records[2].txtlen != 9)
        return 14;
    for (int i = 0; i < 3; ++i) {
        if (late_records[i].seekadr !=
                (const unsigned char *)(unsigned int)((i + 1) * 0x1111u) ||
            late_records[i].txtlen != (i + 1) * 11)
            return 15;
    }
    return 0;
}
