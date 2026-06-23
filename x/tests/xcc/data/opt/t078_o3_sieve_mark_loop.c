typedef unsigned char u8;

static u8 prime[128];

int
main(void)
{
    u8 p;
    u8 j;

    for (p = 0u; p < 128u; ++p)
        prime[p] = 1u;
    prime[0] = 0u;
    prime[1] = 0u;

    for (p = 2u; p < 128u; ++p) {
        if (prime[p] == 0u)
            continue;
        j = (u8)(p + p);
        while (j < 128u) {
            prime[j] = 0u;
            j = (u8)(j + p);
        }
    }

    return (int)prime[127];
}
