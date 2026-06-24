void tame_srl_a(void) {
    __asm__(
        "srl a\n"
        "srl a\n"
        "srl a\n"
        "srl a\n"
        "srl a\n"
        "xor d\n"
        "srl a\n"
        "srl a\n"
        "srl a\n"
        "srl a\n"
        "srl a\n"
        "and #1");
}
