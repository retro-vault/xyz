struct sample {
    unsigned char head;
    int arg;
    unsigned char tail;
};

__attribute__((noinline)) static int
observe(const struct sample *value, int expected)
{
    return value->head == 0x5a
        && value->arg == expected
        && value->tail == 0xa5;
}

__attribute__((noinline)) static int
forward_direct(int arg)
{
    struct sample value;

    value.head = 0x5a;
    value.arg = arg;
    value.tail = 0xa5;
    return observe(&value, arg);
}

__attribute__((noinline)) static int
forward_alias(int arg)
{
    struct sample value;
    struct sample *alias = &value;

    alias->head = 0x5a;
    alias->arg = arg;
    alias->tail = 0xa5;
    return observe(alias, arg);
}

int
main(void)
{
    if (!forward_direct(0x1234))
        return 1;
    if (!forward_direct(-1234))
        return 2;
    if (!forward_alias(0x5678))
        return 3;
    if (!forward_alias(-5678))
        return 4;
    return 0;
}
