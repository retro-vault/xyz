void dragon_flags_more(void) {
    __asm__(
        "cp #1\n"
        "xor #2\n"
        "bit 3, a\n"
        "sub #0\n"
        "add a, #0\n"
        "cp #4\n"
        "xor #0\n"
        "and #7");
}
