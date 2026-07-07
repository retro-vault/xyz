#define BUF_LEN 1024
#define REPS    63

static unsigned char buffer[BUF_LEN];

static unsigned int
crc16_ccitt(unsigned char *data, unsigned int len)
{
    unsigned int crc = 0xFFFFu;
    unsigned char *end = data + len;

    while (data < end) {
        crc ^= ((unsigned int)*data++) << 8;
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
    }

    return crc;
}

static unsigned int
mix_table(unsigned int *tab, unsigned int n)
{
    unsigned int a = 0x6745u;
    unsigned int b = 0xEFCDu;
    unsigned int c = 0x98BAu;
    unsigned int d = 0x1032u;
    unsigned int i;

    for (i = 0; i < n; i++) {
        unsigned int t = tab[i & 0x0Fu];

        a = a + ((b & c) | ((~b) & d)) + t + 0x7891u;
        a = (a << 5) | (a >> 11);
        d = c;
        c = b;
        b = a;
        a = d;
    }

    return a ^ b ^ c ^ d;
}

static void
init_data(void)
{
    unsigned int i;
    unsigned int seed = 0xC001u;

    for (i = 0; i < BUF_LEN; i++) {
        buffer[i] = (unsigned char)(seed & 0xFFu);
        seed = (unsigned int)(seed * 25173u + 13849u);
    }
}

int
main(void)
{
    unsigned int crc = 0;
    unsigned int rep;
    unsigned int mixed;

    init_data();
    for (rep = 0; rep < REPS; rep++)
        crc ^= crc16_ccitt(buffer, BUF_LEN);

    if (crc != 0xEE7Eu)
        return 1;

    mixed = mix_table((unsigned int *)buffer, 4096u);
    if (mixed != 0xF37Bu)
        return 2;
    return 0;
}
