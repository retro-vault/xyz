static int add_bias(int x) {
    return x + 5;
}

int use_a(void) {
    return add_bias(3);
}

int use_b(void) {
    return add_bias(3);
}
