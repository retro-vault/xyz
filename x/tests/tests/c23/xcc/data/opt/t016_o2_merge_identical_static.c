static int add1a(int x) {
    return x + 1;
}

static int add1b(int x) {
    return x + 1;
}

int use_merge(int a, int b) {
    return add1a(a) + add1b(b);
}
