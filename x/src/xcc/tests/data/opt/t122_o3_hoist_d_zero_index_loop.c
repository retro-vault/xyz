unsigned char hoist_buf[16];

void dragon_hoist_d_zero(void) {
    __asm__(
        "ld e, #0\n"
        "hoist_d_zero_loop:\n"
        "ld a, e\n"
        "ld hl, #_hoist_buf\n"
        "ld d, #0\n"
        "add hl, de\n"
        "ld (hl), a\n"
        "inc e\n"
        "ld a, e\n"
        "sub #16\n"
        "jr c, hoist_d_zero_loop");
}
