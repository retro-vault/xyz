#define N       512
#define LOOKUPS 1000
#define REPS    6
#define HITS    1514u
#define CHK     60689u

static int table[N];

static int
bsearch_idx(int key)
{
    int lo = 0;
    int hi = N - 1;

    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        int v = table[mid];

        if (v == key)
            return mid;
        if (v < key)
            lo = mid + 1;
        else
            hi = mid - 1;
    }

    return -1;
}

int
main(void)
{
    unsigned int hits = 0;
    unsigned int chk = 0;
    unsigned int lcg;
    int r;
    int q;
    int idx;

    for (r = 0; r < N; r++)
        table[r] = r * 3;

    for (r = 0; r < REPS; r++) {
        lcg = (unsigned int)(0x51EDu + r);
        for (q = 0; q < LOOKUPS; q++) {
            int key;

            lcg = (unsigned int)((lcg * 181u + 17u) & 0xffffu);
            key = (int)(lcg & 0x7ffu);
            idx = bsearch_idx(key);
            if (idx >= 0) {
                hits++;
                chk = (unsigned int)((chk + (unsigned int)(idx + 1)) & 0xffffu);
            }
        }
    }

    if (hits != HITS)
        return 1;
    if (chk != CHK)
        return 2;
    return 0;
}
