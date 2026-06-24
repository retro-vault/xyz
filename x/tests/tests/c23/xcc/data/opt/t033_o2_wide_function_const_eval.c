extern unsigned int __mul16(unsigned int a, unsigned int b);

int f(void) {
    unsigned int a = 9u;
    unsigned int b = 9u;
    return (int)__mul16(a, b);
}
