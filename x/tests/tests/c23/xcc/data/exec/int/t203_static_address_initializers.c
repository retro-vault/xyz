/* Regression: static pointer initializers must retain symbol relocations. */
static int object;
static int values[3];

struct record {
    unsigned char tag;
    int value;
};

static struct record item;

static void callback(void)
{
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
    if (function_ptrs[0] != &callback)
        return 5;
    if (function_ptrs[1] != callback)
        return 6;
    if (scalar_ptr != &object)
        return 7;
    if (entries[0] != &object)
        return 8;
    if (entries[1] != &values[1])
        return 9;
    return 0;
}
