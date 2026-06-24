void tame_os(void) {
    __asm__(
        "and #0\n"
        "neg\n"
        "set 0, l\n"
        "dec hl\n"
        "ccf\n"
        "cp #42");
}
