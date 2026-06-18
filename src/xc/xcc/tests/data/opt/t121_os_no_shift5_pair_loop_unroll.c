void tame_shift5_pair(void) {
    __asm__(
        "ld b, #5\n"
        "shift5_pair_loop_os:\n"
        "srl l\n"
        "rr a\n"
        "djnz shift5_pair_loop_os\n"
        "xor e\n"
        "ld b, #0");
}
