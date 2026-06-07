static int inc(int x) {
    return x + 1;
}

static int twice_inc(int x) {
    return inc(inc(x));
}

int f(void) {
    return twice_inc(40);
}
