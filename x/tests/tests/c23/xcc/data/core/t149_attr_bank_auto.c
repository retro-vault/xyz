int f(void) {
    [[xcc::bank(3)]] int local = 1;
    return local;
}
