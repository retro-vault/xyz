// Tests IR strength reduction at -O2: unsigned mul/div by power-of-two -> shift
unsigned int f(unsigned int x) {
    return x * 4 + x / 8;
}
