extern void tty_puts(const char *);

static void local_buf_emit(char ch) {
    char buf[2];
    buf[0] = ch;
    buf[1] = 0;
    tty_puts(buf);
}

void wrap_emit_pair(char ch) {
    local_buf_emit(ch);
}
