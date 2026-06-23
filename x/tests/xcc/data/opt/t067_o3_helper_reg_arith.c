unsigned int f(unsigned int acc, unsigned int value) {
    acc ^= (unsigned int)(value + 0x9e37u);
    acc = (unsigned int)((acc << 5) | (acc >> 11));
    acc += (unsigned int)(value ^ 0x7f4au);
    return acc;
}
