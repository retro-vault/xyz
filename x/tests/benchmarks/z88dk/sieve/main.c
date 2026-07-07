#define SIZE 8000

static unsigned char flags[SIZE];

static unsigned int
sieve_count(void)
{
    unsigned int i;
    unsigned int i_sq;
    unsigned int k;
    unsigned int count;

    for (i = 0; i < SIZE; ++i)
        flags[i] = 0;

    count = SIZE - 2;
    i_sq = 4;
    for (i = 2; i_sq < SIZE; ++i) {
        if (!flags[i]) {
            for (k = i_sq; k < SIZE; k += i) {
                count -= !flags[k];
                flags[k] = 1;
            }
        }
        i_sq += i + i + 1;
    }

    return count;
}

int
main(void)
{
    if (sieve_count() != 1007u)
        return 1;
    return 0;
}
