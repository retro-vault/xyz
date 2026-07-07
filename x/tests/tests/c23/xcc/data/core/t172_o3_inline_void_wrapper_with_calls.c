extern void tty_putc(int);

static void emit_text(const char *s) {
    while (*s) {
        tty_putc(*s);
        s++;
    }
}

void wrap_emit(const char *s) {
    emit_text(s);
}
