unsigned int below_1024(unsigned int value)
{
    if (value < 1024u)
        return value + 1u;
    return 0u;
}

unsigned int at_least_1024(unsigned int value)
{
    if (value >= 1024u)
        return value - 1u;
    return 0u;
}
