int f(int x, int y) {
    if (x) {
        y = y + 1;
        y = y ^ 3;
        return y + 7;
    } else {
        y = y - 1;
        y = y ^ 3;
        return y + 7;
    }
}
