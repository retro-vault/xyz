static int plus1(int x) { return x + 1; }

int use_plus1_twice(int x, int y) {
    return plus1(x) + plus1(y);
}
