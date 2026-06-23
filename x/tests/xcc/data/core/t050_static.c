// Test: static local storage duration and static file-scope linkage
int counter(void) {
    static int n = 0;
    n = n + 1;
    return n;
}

static int double_it(int x) {
    return x * 2;
}

int use_double(int x) {
    return double_it(x);
}
