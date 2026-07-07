unsigned char bump_local_guard(unsigned char x) {
    unsigned char g = x;
    if (++g == 0)
        return 1;
    return 0;
}
