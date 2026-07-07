typedef unsigned char uint8_t;

static void copy_prefix(char *dst, const char *src) {
    uint8_t i = 0;
    while (i < 3 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

void wrap_copy(char *dst, const char *src) {
    copy_prefix(dst, src);
}
