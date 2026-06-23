void f(void) {
    __asm__(
        "ld c, a\n"
        "ld b, #0\n"
        "ld h, b\n"
        "ld l, c\n"
        "ld a, h\n"
        "or a, l\n"
        "jr z, done\n"
        "done:");
}
