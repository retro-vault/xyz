int parser_consume(int value)
{
    int total = 0;

    while (value-- > 0)
        total += value & 3;
    return total;
}

int parser_forward(int value)
{
    volatile int checked = value;

    if (checked < 0)
        return 0;
    return parser_consume(checked);
}
