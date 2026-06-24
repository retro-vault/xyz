void dragon_acc(void) {
    __asm__(
        "and #0\n"
        "neg\n"
        "add a, a\n"
        "rr a\n"
        "sla a\n"
        "rr a\n"
        "srl a\n"
        "rl a\n"
        "and #255\n"
        "rr a\n"
        "add a, #128\n"
        "or a\n"
        "scf\n"
        "adc a, #0\n"
        "ld a, #0\n"
        "srl a");
}
