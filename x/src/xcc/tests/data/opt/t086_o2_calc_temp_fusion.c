int f(int a, int b, int c, int d) {
    return (((a * b + c) * (d - 2) + (a - b) * c + 42) << 1) | (a & b);
}
