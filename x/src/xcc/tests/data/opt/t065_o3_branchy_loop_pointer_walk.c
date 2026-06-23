static unsigned char src[16];
static unsigned char dst[16];

void f(void) {
    unsigned int i;

    for (i = 0; i < 16u; ++i) {
        if (src[i] & 1u)
            dst[i] = src[i];
        else
            dst[i] = (unsigned char)(src[i] ^ 1u);
    }
}
