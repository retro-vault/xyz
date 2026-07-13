unsigned int
adjust_predicate(unsigned int value, unsigned char predicate)
{
    value = value - (predicate == 0u);
    value = value + (predicate == 7u);
    return value;
}
