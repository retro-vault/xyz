unsigned framed_one(unsigned value)
{
    volatile unsigned local = value + 1u;
    return local;
}

unsigned framed_two(unsigned value)
{
    volatile unsigned local = value + 2u;
    return local;
}

unsigned framed_three(unsigned value)
{
    volatile unsigned local = value + 3u;
    return local;
}
