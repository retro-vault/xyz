void dragon_acc_more(void) {
    __asm__(
        "xor #255\n"
        "sbc a, #255\n"
        "cpl\n"
        "neg\n"
        "add a, #0\n"
        "rl a\n"
        "res 7, a\n"
        "and a\n"
        "set 0, a\n"
        "or a");
}
