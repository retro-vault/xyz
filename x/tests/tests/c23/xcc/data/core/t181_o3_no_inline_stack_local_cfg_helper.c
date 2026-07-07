static void copy7(const char *src, char out[8]) {
    unsigned char i = 0;

    while (i < 7 && src[i] != '\0') {
        out[i] = src[i];
        i++;
    }

    out[i] = '\0';
}

char use_copy7(const char *src) {
    char out[8];

    copy7(src, out);
    return out[0];
}
