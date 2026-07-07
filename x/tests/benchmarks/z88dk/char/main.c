#define BUF_LEN 1024
#define REPS    63

static unsigned char buffer[BUF_LEN];

static unsigned char
crc8_ccitt(unsigned char *data, unsigned int len)
{
    unsigned char crc = 0xFFu;
    unsigned char *end = data + len;

    while (data < end) {
        crc ^= *data++;
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
        crc = (crc & 0x80u) ? ((unsigned char)(crc << 1) ^ 0x07u) : (unsigned char)(crc << 1);
    }

    return crc;
}

static signed char
schar_mix(signed char *data, unsigned int len)
{
    signed char acc = -1;
    signed char *end = data + len;

    while (data < end) {
        signed char b = *data++;

        if (b < 0)
            acc = (signed char)(acc - b);
        else
            acc = (signed char)(acc + b);
        acc = (signed char)((acc << 1) ^ b);
    }

    return acc;
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
    unsigned char crc = 0;
    signed char mix = 0;
    unsigned int rep;

    init_data();
    for (rep = 0; rep < REPS; rep++)
        crc ^= crc8_ccitt(buffer, BUF_LEN);

    if (crc != 0x4Eu)
        return 1;

    for (rep = 0; rep < REPS; rep++)
        mix = (signed char)(mix ^ schar_mix((signed char *)buffer, BUF_LEN));

    if ((unsigned char)mix != 0xB2u)
        return 2;
    return 0;
}
