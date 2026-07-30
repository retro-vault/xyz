struct triple {
    int first;
    int second;
    int third;
};

static __attribute__((noinline)) struct triple
make_triple(int value)
{
    struct triple result;
    result.first = value;
    result.second = value + 1;
    result.third = value + 2;
    return result;
}

static __attribute__((noinline)) int
sum_triple(void)
{
    struct triple value = make_triple(10);
    return value.first + value.second + value.third;
}

int
main(void)
{
    return sum_triple() == 33 ? 0 : 1;
}
