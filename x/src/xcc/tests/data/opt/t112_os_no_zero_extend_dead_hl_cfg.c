void f(void) {
    __asm__(
        "ld c, a\n"
        "ld b, #0\n"
        "ld h, b\n"
        "ld l, c\n"
        "or a, a\n"
        "jr z, zero\n"
        "ld hl, #1\n"
        "jp done\n"
        "zero:\n"
        "cp #7\n"
        "jr z, other\n"
        "ld hl, #2\n"
        "jp done\n"
        "other:\n"
        "ld hl, #3\n"
        "done:");
}
