static int sum_to(int n) {
    int s = 0;

    while (n > 0) {
        s = s + n;
        n = n - 1;
    }

    return s;
}

int use_a(void) {
    return sum_to(4);
}

int use_b(void) {
    return sum_to(6);
}
