// Tests [[xcc::bank(N)]]: functions and static-storage objects go into banked sections.
[[xcc::bank(7)]] int banked_global = 3;

[[xcc::bank(5)]] int banked_add(int x) {
    return x + banked_global;
}

int owner(void) {
    [[xcc::bank(9)]] static unsigned char sticky = 4;
    return banked_add(sticky);
}
