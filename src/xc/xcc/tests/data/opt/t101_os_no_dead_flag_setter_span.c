void tame_os_dead_flag_span(void) {
    __asm__(
        "xor #0\n"
        "ld h, a\n"
        "ld -1(ix), h\n"
        "ld a, #9\n"
        "cp #7\n"
        "or a\n"
        "ld b, h\n"
        "ld c, l\n"
        "sub #1");
}
