void dragon_shift5_pair(void) {
    __asm__(
        "ld b, #5\n"
        "shift5_pair_loop:\n"
        "srl l\n"
        "rr a\n"
        "djnz shift5_pair_loop\n"
        "xor e\n"
        "ld b, #0");
}
