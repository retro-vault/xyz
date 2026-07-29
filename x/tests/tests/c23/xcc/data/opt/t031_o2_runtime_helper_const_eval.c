extern unsigned int _mul16(unsigned int a, unsigned int b);

static unsigned int square(unsigned int x) {
    return _mul16(x, x);
}

int f(void) {
    return (int)square(9u);
}
