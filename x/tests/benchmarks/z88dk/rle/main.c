#define N    1024
#define REPS 64

static unsigned char inb[N];
static unsigned char outb[2 * N];

static void
fill(void)
{
    unsigned int i = 0;
    unsigned int j;
    unsigned int runlen;
    unsigned char s = 0xA5u;
    unsigned char v;

    while (i < N) {
        s = (unsigned char)(s * 181u + 1u);
        runlen = (unsigned int)((s & 15u) + 1u);
        v = (unsigned char)((s >> 4) & 3u);
        for (j = 0; j < runlen && i < N; j++)
            inb[i++] = v;
    }
}

static unsigned int
rle_encode(void)
{
    unsigned int i = 0;
    unsigned int o = 0;
    unsigned int run;
    unsigned char v;

    while (i < N) {
        v = inb[i];
        run = 1;
        i++;
        while (i < N && inb[i] == v && run < 255) {
            run++;
            i++;
        }
        outb[o++] = (unsigned char)run;
        outb[o++] = v;
    }

    return o;
}

int
main(void)
{
    unsigned int o = 0;
    unsigned int chk = 0;
    unsigned int r;
    unsigned int k;

    fill();
    for (r = 0; r < REPS; r++)
        o = rle_encode();

    for (k = 0; k < o; k++)
        chk = (chk + outb[k]) & 0xffffu;

    if (o != 200u)
        return 1;
    if (chk != 1174u)
        return 2;
    return 0;
}
