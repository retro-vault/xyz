void dragon_pair(void) {
    __asm__(
        "set 0, l\n"
        "dec hl\n"
        "set 0, e\n"
        "dec de\n"
        "set 0, c\n"
        "dec bc\n"
        "res 0, l\n"
        "inc hl\n"
        "res 0, e\n"
        "inc de\n"
        "res 0, c\n"
        "inc bc");
}
