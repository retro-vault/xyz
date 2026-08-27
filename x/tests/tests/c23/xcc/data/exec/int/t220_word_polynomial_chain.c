static const unsigned char standard_vector[9] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

static __attribute__((noinline)) unsigned int
crc16_ccitt(const unsigned char *data, unsigned int length)
{
    const unsigned char *end = data + length;
    unsigned int crc = 0xffffu;

    while (data < end) {
        crc ^= (unsigned int)*data++ << 8;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
        crc = (crc & 0x8000u) ? (crc << 1) ^ 0x1021u : crc << 1;
    }
    return crc;
}

int
main(void)
{
    if (crc16_ccitt(standard_vector, 0u) != 0xffffu)
        return 1;
    if (crc16_ccitt(standard_vector, 9u) != 0x29b1u)
        return 2;
    if (crc16_ccitt(standard_vector + 1, 7u) != 0x60d4u)
        return 3;
    return 0;
}
