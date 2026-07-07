extern void sink(int);

void prints_like(const char *p, char pad, unsigned char width) {
    if (width > 0)
        sink(pad);
    if (*p)
        sink(*p);
    if (width > 0)
        sink(pad);
}
