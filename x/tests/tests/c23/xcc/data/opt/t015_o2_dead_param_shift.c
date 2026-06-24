static int keep_second(int unused, int y) {
    return y + 1;
}

int use_second(int x) {
    return keep_second(99, x);
}
