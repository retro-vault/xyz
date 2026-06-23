void tame_os_register_moves(void) {
    __asm__(
        "ld a, h\n"
        "ld h, a\n"
        "ld a, #3\n"
        "ld b, #1\n"
        "ld b, c\n"
        "ld d, e\n"
        "ld e, d");
}
