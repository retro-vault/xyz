#define BUF_LEN 1024
#define REPS    63

static unsigned char buffer[BUF_LEN];

static unsigned long
crc32(unsigned char *data, unsigned int len)
{
    unsigned long crc = 0xFFFFFFFFul;
    unsigned char *end = data + len;

    while (data < end) {
        crc ^= (unsigned long)*data++;
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
        crc = (crc & 1ul) ? ((crc >> 1) ^ 0xEDB88320ul) : (crc >> 1);
    }

    return crc ^ 0xFFFFFFFFul;
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
    unsigned long crc = 0;
    unsigned int rep;

    init_data();
    for (rep = 0; rep < REPS; rep++)
        crc ^= crc32(buffer, BUF_LEN);

    if (crc != 0x1C48DD57ul)
        return 1;
    return 0;
}
