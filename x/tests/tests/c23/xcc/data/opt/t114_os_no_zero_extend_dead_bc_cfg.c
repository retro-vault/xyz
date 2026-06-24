void f(void) {
    __asm__(
        "ld c, a\n"
        "ld b, #0\n"
        "or a, a\n"
        "jr z, zero\n"
        "ld b, h\n"
        "ld c, l\n"
        "jp done\n"
        "zero:\n"
        "cp #7\n"
        "jr z, other\n"
        "ld c, #1\n"
        "ld b, #0\n"
        "jp done\n"
        "other:\n"
        "ld bc, #2\n"
        "done:");
}
