int f(void) {
    unsigned long value = 0x12345678ul;
    unsigned int *words = (unsigned int *)&value;

    if (words[0] != 0x5678u) return 1;
    if (words[1] != 0x1234u) return 2;
    return 0;
}
