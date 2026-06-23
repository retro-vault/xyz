void dragon_flags(void) {
    __asm__(
        "ccf\n"
        "cp #42\n"
        "scf\n"
        "xor #1\n"
        "or a\n"
        "cp #7\n"
        "and a\n"
        "sub #3");
}
