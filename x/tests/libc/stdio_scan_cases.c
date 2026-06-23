/*
 * stdio_scan_cases.c
 *
 * Integration checks for the assembly-only scanf family.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern void __sys_getchar_reset(void);
extern void __sys_getchar_setbuf(const char *s);
extern void __sys_file_reset(void);
extern int  __sys_file_mount(const char *name, char *buf,
                             unsigned int len, unsigned int cap);
extern FILE *__stdio_stdin_handle(void);

static char scan_file_buf[32] = "11 2a";
static int scan_a, scan_b, scan_i, scan_n;
static unsigned scan_o, scan_x;
static char scan_word[8];
static char scan_ch;
static signed char scan_sc;
static long scan_lv;
static unsigned long long scan_ull;
static void *scan_ptr;
static float scan_fa, scan_fb;
static double scan_da;

static int scan_streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

typedef union scan_float_bits {
    float f;
    unsigned long u;
} scan_float_bits;

typedef union scan_double_bits {
    double d;
    unsigned long long u;
} scan_double_bits;

static int wrap_vsscanf(const char *src, const char *fmt, ...) {
    va_list ap;
    int rc;
    va_start(ap, fmt);
    rc = vsscanf(src, fmt, ap);
    va_end(ap);
    return rc;
}

static int wrap_vfscanf(FILE *stream, const char *fmt, ...) {
    va_list ap;
    int rc;
    va_start(ap, fmt);
    rc = vfscanf(stream, fmt, ap);
    va_end(ap);
    return rc;
}

static int wrap_vscanf(const char *fmt, ...) {
    va_list ap;
    int rc;
    va_start(ap, fmt);
    rc = vscanf(fmt, ap);
    va_end(ap);
    return rc;
}

static int wrap_fscanf_float_pair(FILE *stream, float *a, double *d) {
    return fscanf(stream, "%f %lf", a, d);
}

static int scan_string_cases(void) {
    scan_a = scan_b = scan_i = scan_n = 0;
    scan_o = scan_x = 0;
    scan_word[0] = 0;
    scan_ch = 0;

    scan_a = 0;
    scan_b = 0x1111;
    scan_i = 0x2222;
    scan_o = 0x3333u;
    if (sscanf(" -12", "%d", &scan_a) != 1) return 90;
    if (scan_a != -12) {
        if (scan_b == (int)-12) return 411;
        if (scan_i == (int)-12) return 412;
        if (scan_o == (unsigned)-12) return 413;
        return 400 + (scan_a & 0xff);
    }

    {
        int rc = sscanf(" -12 34 0x1f 077 test Q",
                        "%d %u %i %o %4s %c",
                        &scan_a, &scan_b, &scan_i, &scan_o,
                        scan_word, &scan_ch);
        if (rc != 6) return 100 + rc;
    }
    if (scan_a != -12) {
        if (scan_b == (unsigned)-12) return 211;
        if (scan_i == -12) return 212;
        if (scan_o == (unsigned)-12) return 213;
        return 300 + (scan_a & 0xff);
    }
    if (scan_b != 34) return 202;
    if (scan_i != 31) return 203;
    if (scan_o != 63) return 204;
    if (!scan_streq(scan_word, "test")) return 3;
    if (scan_ch != 'Q') return 31;

    scan_n = -1;
    scan_word[0] = 0;
    {
        int rc = sscanf("123 abc", "%*d %n%3s", &scan_n, scan_word);
        if (rc != 1) return 40 + rc;
    }
    if (scan_n != 4) return 5;
    if (!scan_streq(scan_word, "abc")) return 6;

    scan_sc = 0;
    scan_lv = 0;
    scan_ull = 0ULL;
    if (wrap_vsscanf("-5 123456 1234abcd", "%hhd %ld %llx",
                     &scan_sc, &scan_lv, &scan_ull) != 3) return 7;
    if (scan_sc != (signed char)-5) return 8;
    if (scan_lv != 123456L) return 9;
    if (scan_ull != 0x1234abcdULL) return 10;

    return 0;
}

static int scan_pointer_and_float_cases(void) {
    scan_ptr = 0;
    if (sscanf("0x1234", "%p", &scan_ptr) != 1) return 11;
    if ((unsigned int)(unsigned long)scan_ptr != 0x1234u) return 12;

    scan_fa = 0.0f;
    scan_da = 0.0;
    scan_fb = 0.0f;
    if (sscanf(" -1.25 2.5e1 6.25e-2", "%f %lf %g",
               &scan_fa, &scan_da, &scan_fb) != 3) return 25;
    {
        scan_float_bits fa, fb;
        scan_double_bits da;
        fa.f = scan_fa;
        da.d = scan_da;
        fb.f = scan_fb;
        if (fa.u != 0xbfa00000UL) return 26;
        if (da.u != 0x4039000000000000ULL) return 27;
        if (fb.u != 0x3d800000UL) return 28;
    }

    scan_da = 0.0;
    if (wrap_vsscanf("7.5e-1 tail", "%le", &scan_da) != 1) return 29;
    {
        scan_double_bits da;
        da.d = scan_da;
        if (da.u != 0x3fe8000000000000ULL) return 30;
    }

    return 0;
}

static int scan_stdin_cases(void) {
    __sys_getchar_reset();
    __stdio_stdin_handle();
    __sys_getchar_setbuf("55 xy");
    scan_a = 0;
    scan_word[0] = 0;
    {
        int rc = scanf("%d %2s", &scan_a, scan_word);
        if (rc != 2) return 130 + rc;
    }
    if (scan_a != 55 || !scan_streq(scan_word, "xy")) return 14;

    __sys_getchar_reset();
    __stdio_stdin_handle();
    __sys_getchar_setbuf("77 ok");
    scan_a = 0;
    scan_word[0] = 0;
    if (wrap_vscanf("%d %2s", &scan_a, scan_word) != 2) return 15;
    if (scan_a != 77 || !scan_streq(scan_word, "ok")) return 16;

    __sys_getchar_reset();
    __stdio_stdin_handle();
    __sys_getchar_setbuf("4.5 6.75e1");
    scan_fa = 0.0f;
    scan_da = 0.0;
    if (scanf("%f %lf", &scan_fa, &scan_da) != 2) return 31;
    {
        scan_float_bits fa;
        scan_double_bits da;
        fa.f = scan_fa;
        da.d = scan_da;
        if (fa.u != 0x40900000UL) return 32;
        if (da.u != 0x4050e00000000000ULL) return 33;
    }
    return 0;
}

static int scan_file_cases(void) {
    FILE *f;

    __sys_file_reset();
    scan_file_buf[0] = '1';
    scan_file_buf[1] = '1';
    scan_file_buf[2] = ' ';
    scan_file_buf[3] = '2';
    scan_file_buf[4] = 'a';
    scan_file_buf[5] = '\0';
    if (__sys_file_mount("scan.txt", scan_file_buf, 5u, sizeof(scan_file_buf)) < 0) return 17;
    f = fopen("scan.txt", "r");
    if (!f) return 18;
    scan_o = scan_x = 0;
    if (fscanf(f, "%o %x", &scan_o, &scan_x) != 2) return 19;
    if (scan_o != 9u || scan_x != 0x2au) return 20;
    if (fseek(f, 0L, SEEK_SET) != 0) return 21;
    scan_o = scan_x = 0;
    {
        int rc = wrap_vfscanf(f, "%o %x", &scan_o, &scan_x);
        if (rc != 2) return 120 + rc;
    }
    if (scan_o != 9u || scan_x != 0x2au) return 23;
    if (fclose(f) != 0) return 24;

    scan_file_buf[0] = '3';
    scan_file_buf[1] = '.';
    scan_file_buf[2] = '1';
    scan_file_buf[3] = '2';
    scan_file_buf[4] = '5';
    scan_file_buf[5] = ' ';
    scan_file_buf[6] = '-';
    scan_file_buf[7] = '2';
    scan_file_buf[8] = '.';
    scan_file_buf[9] = '5';
    scan_file_buf[10] = 'e';
    scan_file_buf[11] = '1';
    scan_file_buf[12] = '\0';
    if (__sys_file_mount("scanf.txt", scan_file_buf, 12u, sizeof(scan_file_buf)) < 0) return 34;
    f = fopen("scanf.txt", "r");
    if (!f) return 35;
    scan_fa = 0.0f;
    scan_da = 0.0;
    if (wrap_fscanf_float_pair(f, &scan_fa, &scan_da) != 2) return 36;
    {
        scan_float_bits fa;
        scan_double_bits da;
        fa.f = scan_fa;
        da.d = scan_da;
        if (fa.u != 0x40480000UL) return 37;
        if (da.u != 0xc039000000000000ULL) return 38;
    }
    if (fclose(f) != 0) return 39;

    return 0;
}

int stdio_scan_cases(void) {
    int rc;

    rc = scan_string_cases();
    if (rc) return rc;
    rc = scan_pointer_and_float_cases();
    if (rc) return rc;
    rc = scan_stdin_cases();
    if (rc) return rc;
    return scan_file_cases();
}
