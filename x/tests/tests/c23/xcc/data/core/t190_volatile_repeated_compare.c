volatile unsigned int probe;

int
volatile_second_failure(void)
{
    return 2;
}

int
check_probe(void)
{
    if (probe != 7u)
        return 1;
    if (probe != 7u)
        return volatile_second_failure();
    return 0;
}
