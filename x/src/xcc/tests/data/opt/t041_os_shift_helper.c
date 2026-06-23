unsigned shift_twice(unsigned x, unsigned a, unsigned b) {
    return (x << a) ^ (x << b);
}
