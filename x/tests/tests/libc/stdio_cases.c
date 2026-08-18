/*
 * stdio_cases.c
 *
 * Integration checks for the assembly-only stdio formatter. The test runs
 * inside the Z80 emulator and exercises the real stack-only variadic ABI.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <wchar.h>
#include <fenv.h>

extern void __sys_putchar_reset(void);
extern int  __sys_putchar_getcount(void);
extern int  __sys_putchar_getchar(int index);
extern void __sys_getchar_reset(void);
extern void __sys_getchar_setbuf(const char *s);
extern void __sys_file_reset(void);
extern int  __sys_file_mount(const char *name, char *buf,
                             unsigned int len, unsigned int cap);
extern FILE *__stdio_stdin_handle(void);
extern FILE *__stdio_stdout_handle(void);
extern FILE *__stdio_stderr_handle(void);

static char stdio_in_file[16] = "alpha";
static char stdio_io_file[32] = "seed";
static char stdio_big_buf[96];
static char stdio_small_buf[8];
static fpos_t stdio_saved_pos;

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

static int sysbuf_eq(const char *s) {
    int i = 0;
    while (s[i]) {
        if (__sys_putchar_getchar(i) != (unsigned char)s[i]) return 0;
        ++i;
    }
    return __sys_putchar_getcount() == i;
}

static int wrap_vsnprintf(char *buf, unsigned int n, const char *fmt, ...) {
    va_list ap;
    int rc;
    va_start(ap, fmt);
    rc = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return rc;
}

int stdio_format_cases(FILE *out, FILE *err) {
    int count_mark;
    int rc;

    __sys_putchar_reset();
    rc = printf("A:%d U:%u X:%x O:%o C:%c S:%s %%\n",
                -12, 34u, 0x2a, 010u, 'Q', "hi");
    if (rc != 32) return 1;
    if (!sysbuf_eq("A:-12 U:34 X:2a O:10 C:Q S:hi %\n")) return 2;

    __sys_putchar_reset();
    rc = printf("L:%ld LL:%llx\n", -123456L, 0x123456789abcdef0ULL);
    if (rc != 30) return 3;
    if (!sysbuf_eq("L:-123456 LL:123456789abcdef0\n")) return 4;

    rc = sprintf(stdio_big_buf, "%05d|%-6s|%.3s|%#x|%#o",
                 42, "hi", "abcdef", 0x3f, 9u);
    if (rc != 25) return 5;
    if (!streq(stdio_big_buf, "00042|hi    |abc|0x3f|011")) return 6;

    rc = snprintf(stdio_big_buf, 5, "abcdef");
    if (rc != 6) return 7;
    if (!streq(stdio_big_buf, "abcd")) return 8;

    count_mark = -1;
    rc = sprintf(stdio_big_buf, "ab%ncd", &count_mark);
    if (rc != 4) return 9;
    if (count_mark != 2) return 10;
    if (!streq(stdio_big_buf, "abcd")) return 11;

    rc = sprintf(stdio_big_buf, "%p", (void *)0x1234);
    if (rc != 6) return 12;
    if (!streq(stdio_big_buf, "0x1234")) return 13;

    rc = wrap_vsnprintf(stdio_big_buf, sizeof(stdio_big_buf),
                        "[%6d][%s][%#X]", 42, "ok", 0xbeef);
    if (rc != 20) return 14;
    if (!streq(stdio_big_buf, "[    42][ok][0XBEEF]")) return 15;

    __sys_putchar_reset();
    if (putchar('Z') != 'Z') return 16;
    if (printf("!%s", " go") != 4) return 17;
    if (puts(" now") < 0) return 19;
    if (!sysbuf_eq("Z! go now\n")) return 20;

    __sys_putchar_reset();
    if (fwrite("ABCD", 2u, 2u, out) != 2u) return 39;
    if (!sysbuf_eq("ABCD")) return 40;
    if (putc('!', err) != '!') return 41;
    if (!sysbuf_eq("ABCD!")) return 42;
    if (fflush(out) != 0) return 43;

    if (fgetc(out) != EOF) return 44;
    if (!ferror(out)) return 45;
    clearerr(out);
    if (ferror(out)) return 46;

    return 0;
}

int stdio_console_input_cases(FILE *in) {
    __sys_getchar_reset();
    __sys_getchar_setbuf("abc\n");
    if (getchar() != 'a') return 21;
    if (ungetc('Z', in) != 'Z') return 22;
    if (getc(in) != 'Z') return 23;
    if (fgetc(in) != 'b') return 24;
    if (fgetc(in) != 'c') return 25;
    if (fgetc(in) != '\n') return 26;
    if (fgetc(in) != EOF) return 27;
    if (!feof(in)) return 28;
    clearerr(in);
    if (feof(in)) return 29;

    __sys_getchar_reset();
    __sys_getchar_setbuf("line one\nline two");
    if (fgets(stdio_big_buf, sizeof(stdio_big_buf), in) != stdio_big_buf) return 30;
    if (!streq(stdio_big_buf, "line one\n")) return 31;
    if (fgets(stdio_big_buf, sizeof(stdio_big_buf), in) != stdio_big_buf) return 32;
    if (!streq(stdio_big_buf, "line two")) return 33;
    if (fgets(stdio_big_buf, sizeof(stdio_big_buf), in) != 0) return 34;

    __sys_getchar_reset();
    __sys_getchar_setbuf("abcdef");
    if (fread(stdio_big_buf, 2u, 2u, in) != 2u) return 35;
    stdio_big_buf[4] = 0;
    if (!streq(stdio_big_buf, "abcd")) return 36;
    if (fread(stdio_big_buf, 1u, 4u, in) != 2u) return 37;
    stdio_big_buf[2] = 0;
    if (!streq(stdio_big_buf, "ef")) return 38;

    return 0;
}

int stdio_file_cases(void) {
    FILE *f;

    __sys_file_reset();
    stdio_in_file[0] = 'a';
    stdio_in_file[1] = 'l';
    stdio_in_file[2] = 'p';
    stdio_in_file[3] = 'h';
    stdio_in_file[4] = 'a';
    stdio_in_file[5] = '\0';
    stdio_io_file[0] = 's';
    stdio_io_file[1] = 'e';
    stdio_io_file[2] = 'e';
    stdio_io_file[3] = 'd';
    stdio_io_file[4] = '\0';

    if (__sys_file_mount("in.txt", stdio_in_file, 5u, sizeof(stdio_in_file)) < 0) return 47;
    if (__sys_file_mount("io.txt", stdio_io_file, 4u, sizeof(stdio_io_file)) < 0) return 48;

    f = fopen("in.txt", "r");
    if (!f) return 49;
    if (fread(stdio_big_buf, 1u, 5u, f) != 5u) return 50;
    stdio_big_buf[5] = 0;
    if (!streq(stdio_big_buf, "alpha")) return 51;
    if (ftell(f) != 5L) return 52;
    rewind(f);
    if (ftell(f) != 0L) return 53;
    if (fseek(f, 2L, SEEK_SET) != 0) return 54;
    if (ftell(f) != 2L) return 55;
    if (fgetc(f) != 'p') return 56;
    if (fgetpos(f, &stdio_saved_pos) != 0) return 57;
    if (stdio_saved_pos != 3L) return 58;
    if (fsetpos(f, &stdio_saved_pos) != 0) return 59;
    if (ftell(f) != 3L) return 60;
    if (fgetc(f) != 'h') return 61;
    if (fclose(f) != 0) return 62;

    f = fopen("io.txt", "w+");
    if (!f) return 63;
    if (fprintf(f, "N=%d", 7) != 3) return 64;
    if (ftell(f) != 3L) return 65;
    if (fgetpos(f, &stdio_saved_pos) != 0) return 66;
    if (stdio_saved_pos != 3L) return 67;
    stdio_saved_pos = 0L;
    if (fsetpos(f, &stdio_saved_pos) != 0) return 68;
    if (fread(stdio_big_buf, 1u, 3u, f) != 3u) return 69;
    stdio_big_buf[3] = 0;
    if (!streq(stdio_big_buf, "N=7")) return 70;
    if (fclose(f) != 0) return 71;
    if (!streq(stdio_io_file, "N=7")) return 72;

    f = fopen("io.txt", "a");
    if (!f) return 73;
    if (fputs("!", f) < 0) return 74;
    if (fclose(f) != 0) return 75;
    if (!streq(stdio_io_file, "N=7!")) return 76;

    if (fopen("missing.txt", "r") != 0) return 77;

    stdio_io_file[0] = 'g';
    stdio_io_file[1] = 'o';
    stdio_io_file[2] = 'n';
    stdio_io_file[3] = 'e';
    stdio_io_file[4] = '\0';
    if (__sys_file_mount("old.txt", stdio_io_file, 4u, sizeof(stdio_io_file)) < 0) return 78;
    if (rename("old.txt", "new.txt") != 0) return 79;
    if (fopen("old.txt", "r") != 0) return 80;
    f = fopen("new.txt", "r");
    if (!f) return 81;
    if (fread(stdio_big_buf, 1u, 4u, f) != 4u) return 82;
    stdio_big_buf[4] = 0;
    if (!streq(stdio_big_buf, "gone")) return 83;
    if (fclose(f) != 0) return 84;
    if (remove("new.txt") != 0) return 85;
    if (fopen("new.txt", "r") != 0) return 86;

    return 0;
}

int stdio_misc_cases(FILE *out) {
    char name1[L_tmpnam];
    char name2[L_tmpnam];

    if (tmpnam(name1) != name1) return 87;
    if (tmpnam(name2) != name2) return 88;
    if (name1[0] == 0) return 89;
    if (streq(name1, name2)) return 90;

    if (setvbuf(out, stdio_big_buf, _IOFBF, sizeof(stdio_big_buf)) != 0) return 91;
    if (setvbuf(out, stdio_big_buf, _IOLBF, sizeof(stdio_big_buf)) != 0) return 92;
    if (setvbuf(out, 0, _IONBF, 0u) != 0) return 93;
    if (setvbuf(out, stdio_big_buf, 99, sizeof(stdio_big_buf)) == 0) return 94;
    setbuf(out, stdio_big_buf);
    setbuf(out, 0);

    __sys_putchar_reset();
    (void)strtol("99999999999999999999", 0, 10);
    perror("math");
    if (!sysbuf_eq("math: range error\n")) return 95;

    __sys_putchar_reset();
    (void)wctomb(stdio_big_buf, (wchar_t)0x1234u);
    perror("");
    if (!sysbuf_eq("illegal byte sequence\n")) return 96;

    return 0;
}

int stdio_tmpfile_case(void) {
    char buf[8];
    FILE *f;

    __sys_file_reset();
    f = tmpfile();
    if (!f) return 1;
    if (fputs("tmp", f) < 0) return 2;
    if (fseek(f, 0L, SEEK_SET) != 0) return 3;
    if (fread(buf, 1u, 3u, f) != 3u) return 4;
    buf[3] = 0;
    if (!streq(buf, "tmp")) return 5;
    if (fclose(f) != 0) return 6;
    return 0;
}

int stdio_freopen_case(void) {
    FILE *f;

    __sys_file_reset();
    stdio_io_file[0] = 's';
    stdio_io_file[1] = 'e';
    stdio_io_file[2] = 'e';
    stdio_io_file[3] = 'd';
    stdio_io_file[4] = '\0';
    if (__sys_file_mount("io.txt", stdio_io_file, 4u, sizeof(stdio_io_file)) < 0) return 1;

    f = fopen("io.txt", "r");
    if (!f) return 2;
    f = freopen("swap.txt", "w+", f);
    if (!f) return 3;
    if (fputs("xy", f) < 0) return 4;
    rewind(f);
    if (fread(stdio_small_buf, 1u, 2u, f) != 2u) return 5;
    stdio_small_buf[2] = 0;
    if (!streq(stdio_small_buf, "xy")) return 6;
    if (fclose(f) != 0) return 7;
    f = fopen("swap.txt", "r");
    if (!f) return 8;
    if (fread(stdio_small_buf, 1u, 2u, f) != 2u) return 9;
    stdio_small_buf[2] = 0;
    if (!streq(stdio_small_buf, "xy")) return 10;
    if (fclose(f) != 0) return 11;
    return 0;
}

static int c23_and_more_calls_case(void);

int stdio_cases(void) {
    FILE *in;
    FILE *out;
    FILE *err;
    int rc;

    in = __stdio_stdin_handle();
    out = __stdio_stdout_handle();
    err = __stdio_stderr_handle();

    rc = stdio_format_cases(out, err);
    if (rc != 0) return rc;
    rc = stdio_console_input_cases(in);
    if (rc != 0) return rc;
    rc = stdio_file_cases();
    if (rc != 0) return rc;
    rc = stdio_misc_cases(out);
    if (rc != 0) return rc;

    // C-driven expansion for "both" (xcc compiled calls to more libc fns incl C23)
    rc = c23_and_more_calls_case();
    if (rc != 0) return rc + 5000;

    return 0;
}

// C-DRIVEN BOTH EXPANSION (see plan): C that xcc lowers to calls of
// strfrom*, C23 math, qsort/bsearch, time, fenv, mb, extra string etc.
static int qsort_cmp(const void *a, const void *b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}

static int c23_and_more_calls_case(void) {
    char buf[96];
    int n = strfromd(buf, sizeof(buf), "%g", 3.14159);
    if (n <= 0) return 1;
    float fmaxv = fmaximumf(2.0f, -5.0f);
    float fr = fromfpf(1.7f, 0, 32);
    (void)fmaxv; (void)fr;
    int data[8] = {7,1,4,3,8,2,5,6};
    qsort(data, 8, sizeof(int), qsort_cmp);
    int key = 4;
    void *hit = bsearch(&key, data, 8, sizeof(int), qsort_cmp);
    if (!hit) return 2;
    wchar_t wc; mbtowc(&wc, "X", 1);
    char mb[8]; wctomb(mb, L'Y');
    time_t now = 1700000000;
    struct tm *tm = gmtime(&now);
    if (tm) strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    feclearexcept(FE_ALL_EXCEPT);
    fesetround(0);
    char *dup = strdup("both");
    if (dup) free(dup);
    strfromf(buf, 16, "%.2f", 0.5f);
    return 0;
}
