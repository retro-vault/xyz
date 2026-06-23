void tame_os_more(void) {
    __asm__(
        "xor #255\n"
        "sbc a, #255\n"
        "cpl\n"
        "neg\n"
        "res 7, a\n"
        "and a\n"
        "cp #1\n"
        "xor #2");
}
