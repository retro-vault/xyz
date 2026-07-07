extern unsigned char probe(void);

int test(void) {
    if (probe())
        return 1;
    return 0;
}
