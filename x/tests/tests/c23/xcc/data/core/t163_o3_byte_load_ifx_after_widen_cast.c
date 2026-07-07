int first_nonzero(const char *p) {
    if (p[0])
        return 1;
    return 0;
}

int indexed_nonzero(const char *p, unsigned char i) {
    if (p[(int)i])
        return 2;
    return 3;
}
