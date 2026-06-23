void f(void) {
    __asm__(
        "ld l, a\n"
        "ld h, #0\n"
        "ld b, h\n"
        "ld c, l\n"
        "ld hl, #1234\n"
        "add hl, bc");
}
