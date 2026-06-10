/*
 * stdio_wide_cases.c
 *
 * Integration checks for the basic wide stdio layer that sits on top of the
 * single-byte execution charset and the fd-backed stdio core.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#include <stdio.h>
#include <wchar.h>

extern void __sys_putchar_reset(void);
extern int  __sys_putchar_getcount(void);
extern int  __sys_putchar_getchar(int index);
extern void __sys_getchar_reset(void);
extern void __sys_getchar_setbuf(const char *s);
extern void __sys_file_reset(void);
extern int  __sys_file_mount(const char *name, char *buf,
                             unsigned int len, unsigned int cap);
extern FILE *__stdio_stdin_handle(void);

static char wide_file_buf[32] = "seed";
static wchar_t wide_buf[16];

static int wide_sysbuf_eq(const char *s) {
    int i = 0;
    while (s[i]) {
        if (__sys_putchar_getchar(i) != (unsigned char)s[i]) return 0;
        ++i;
    }
    return __sys_putchar_getcount() == i;
}

static int wide_wstreq(const wchar_t *a, const wchar_t *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

int stdio_wide_cases(void) {
    FILE *f;

    __sys_putchar_reset();
    if (putwchar(L'Z') != L'Z') return 1;
    if (fputwc(L'!', stdout) != L'!') return 2;
    if (putwc(L'?', stdout) != L'?') return 3;
    if (fputws(L" ok", stdout) < 0) return 4;
    if (!wide_sysbuf_eq("Z!? ok")) return 5;

    __sys_getchar_reset();
    __stdio_stdin_handle();
    __sys_getchar_setbuf("A\n");
    if (getwchar() != L'A') return 6;
    if (ungetwc(L'B', stdin) != L'B') return 7;
    if (getwc(stdin) != L'B') return 8;
    if (fgetwc(stdin) != L'\n') return 9;
    if (fgetwc(stdin) != WEOF) return 10;

    __sys_file_reset();
    wide_file_buf[0] = '\0';
    if (__sys_file_mount("wide.txt", wide_file_buf, 0u, sizeof(wide_file_buf)) < 0) return 11;
    f = fopen("wide.txt", "w+");
    if (!f) return 12;
    if (putwc(L'Q', f) != L'Q') return 13;
    if (fputws(L"RS\n", f) < 0) return 14;
    rewind(f);
    if (getwc(f) != L'Q') return 15;
    if (fgetws(wide_buf, 16, f) != wide_buf) return 16;
    if (!wide_wstreq(wide_buf, L"RS\n")) return 17;
    if (fgetws(wide_buf, 16, f) != 0) return 18;
    if (fclose(f) != 0) return 19;

    return 0;
}
