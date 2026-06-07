extern unsigned int __mul16(unsigned int a, unsigned int b);

static unsigned int square(unsigned int x) {
    return __mul16(x, x);
}

int f(void) {
    return (int)square(9u);
}
