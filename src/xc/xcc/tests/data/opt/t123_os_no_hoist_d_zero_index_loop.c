unsigned char tame_hoist_buf[16];

void tame_hoist_d_zero(void) {
    __asm__(
        "ld e, #0\n"
        "hoist_d_zero_loop_os:\n"
        "ld a, e\n"
        "ld hl, #_tame_hoist_buf\n"
        "ld d, #0\n"
        "add hl, de\n"
        "ld (hl), a\n"
        "inc e\n"
        "ld a, e\n"
        "sub #16\n"
        "jr c, hoist_d_zero_loop_os");
}
