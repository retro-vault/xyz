void f(void) {
    static unsigned char src[8];
    static unsigned char dst[8];
    unsigned int i;

    for (i = 0; i < 8u; ++i) {
        switch (src[i] & 3u) {
        case 0: dst[i] = (unsigned char)(src[i] + 1u); break;
        case 1: dst[i] = (unsigned char)(src[i] ^ 7u); break;
        case 2: dst[i] = (unsigned char)(src[i] & 15u); break;
        default: dst[i] = src[i]; break;
        }
    }
}
