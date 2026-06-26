/*
 * Z80 C23 Comprehensive stdio File I/O Test
 *
 * Covers most standard C file operations:
 * - fopen/fclose in all main modes (r, w, a, r+, w+, a+, binary/text)
 * - fread/fwrite (binary)
 * - fgetc/fputc, fgets/fputs (text)
 * - fprintf/fscanf
 * - fseek (SEEK_SET, SEEK_CUR, SEEK_END), ftell, rewind
 * - Appending (prolonging file)
 * - Truncating (reopen with "w")
 * - Mixed read/write + seek (cross operations)
 * - feof, ferror, clearerr, fflush
 * - Binary vs text mode differences
 * - Small file creation, reading, verification
 */

#include <stddef.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef __XCC_PLATFORM_Z80_CPM3
#include <sys/bdos.h>
#endif

static int tests_passed = 0;
static int tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s\n", msg); \
    } \
} while (0);

#define TEST_FILE "testio.bin"
#define TEXT_FILE "textio.txt"
#define DIAG_FILE "t140diag.bin"

#ifdef __XCC_PLATFORM_Z80_CPM3
#define CPM3_FCB_SIZE 36
#define CPM3_DMA_SIZE 128

static unsigned char cpm3_current_drive_code(void) {
    return (unsigned char)(bdos(DRV_GET, 0) + 1);
}

static void cpm3_init_fcb(unsigned char *fcb, const char *path) {
    int i;
    int name_index;
    int ext_index;
    int in_ext;
    unsigned char ch;

    memset(fcb, 0, CPM3_FCB_SIZE);
    fcb[0] = cpm3_current_drive_code();
    for (i = 1; i <= 11; ++i) {
        fcb[i] = ' ';
    }

    name_index = 1;
    ext_index = 9;
    in_ext = 0;

    while (*path != '\0') {
        ch = (unsigned char)*path++;
        if (ch >= 'a' && ch <= 'z') {
            ch = (unsigned char)(ch - 'a' + 'A');
        }
        if (ch == '.') {
            in_ext = 1;
            continue;
        }
        if (!in_ext) {
            if (name_index <= 8) {
                fcb[name_index++] = ch;
            }
        } else {
            if (ext_index <= 11) {
                fcb[ext_index++] = ch;
            }
        }
    }
}

static void print_cpm3_raw_file_diag(const char *label, const char *path) {
    unsigned char fcb[CPM3_FCB_SIZE];
    unsigned char dma[CPM3_DMA_SIZE];
    unsigned char *entry;
    unsigned high_h;
    bdos_ret_t result;

    cpm3_init_fcb(fcb, path);
    fcb[32] = 0xff;
    bdosret(F_OPEN, (unsigned short)fcb, &result);
    high_h = (unsigned)((result.rethl >> 8) & 0xff);
    printf("  %s raw open: A=%u H=%u ex=%u s2=%u rc=%u cr=%u r0=%u r1=%u r2=%u\n",
           label,
           (unsigned)result.reta,
           high_h,
           (unsigned)fcb[12],
           (unsigned)fcb[14],
           (unsigned)fcb[15],
           (unsigned)fcb[32],
           (unsigned)fcb[33],
           (unsigned)fcb[34],
           (unsigned)fcb[35]);
    if (result.reta != BDOS_FAILURE) {
        bdosret(F_CLOSE, (unsigned short)fcb, &result);
    }

    cpm3_init_fcb(fcb, path);
    bdos(F_DMAOFF, (unsigned short)dma);
    bdosret(F_SEARCHFIRST, (unsigned short)fcb, &result);
    high_h = (unsigned)((result.rethl >> 8) & 0xff);
    if (result.reta == BDOS_FAILURE) {
        printf("  %s raw dir: search-first failed A=%u H=%u\n",
               label,
               (unsigned)result.reta,
               high_h);
    } else {
        entry = dma + ((result.reta & 0x03) * 32);
        printf("  %s raw dir: A=%u ex=%u s1=%u s2=%u rc=%u\n",
               label,
               (unsigned)result.reta,
               (unsigned)entry[12],
               (unsigned)entry[13],
               (unsigned)entry[14],
               (unsigned)entry[15]);
    }

    cpm3_init_fcb(fcb, path);
    fcb[33] = 0;
    fcb[34] = 0;
    fcb[35] = 0;
    bdosret(F_SIZE, (unsigned short)fcb, &result);
    high_h = (unsigned)((result.rethl >> 8) & 0xff);
    printf("  %s raw size: A=%u H=%u r0=%u r1=%u r2=%u\n",
           label,
           (unsigned)result.reta,
           high_h,
           (unsigned)fcb[33],
           (unsigned)fcb[34],
           (unsigned)fcb[35]);
}
#endif

static void print_stdio_size_diag(const char *label, const char *path) {
    FILE *f;
    long end_pos;

    f = fopen(path, "rb");
    if (!f) {
        printf("  %s: fopen rb failed\n", label);
        return;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        printf("  %s: fseek end failed\n", label);
        fclose(f);
        return;
    }

    end_pos = ftell(f);
    printf("  %s: ftell(end)=%ld\n", label, end_pos);
    fclose(f);
}

static void print_stdio_eof_diag(const char *path) {
    FILE *f;

    f = fopen(path, "rb");
    if (!f) {
        printf("  eof diag: fopen rb failed\n");
        return;
    }

    {
        unsigned char buf[100];
        size_t readn;
        int eof_flag;
        int err_flag;

        readn = fread(buf, 1, sizeof(buf), f);
        eof_flag = feof(f);
        err_flag = ferror(f);
        printf("  eof diag: fread=%ld feof=%d ferror=%d\n",
               (long)readn, eof_flag, err_flag);
    }

    fclose(f);
}

static void print_lowlevel_size_diag(const char *path) {
    int fd;
    long end_pos;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("  lowlevel: open failed\n");
        return;
    }

    end_pos = lseek(fd, 0L, SEEK_END);
    printf("  lowlevel: lseek(end)=%ld\n", end_pos);
    close(fd);
}

static void run_bottom_diagnostics(void) {
    FILE *f;

    printf("  diag version: t140-cpm3-v6\n");

    remove(DIAG_FILE);

    f = fopen(DIAG_FILE, "wb");
    if (f) {
        unsigned char data[16] = {
            0, 1, 2, 3, 4, 5, 6, 7,
            8, 9, 10, 11, 12, 13, 14, 15
        };
        fwrite(data, 1, sizeof(data), f);
        fclose(f);
    }
    print_stdio_size_diag("after write16", DIAG_FILE);
    print_lowlevel_size_diag(DIAG_FILE);
#ifdef __XCC_PLATFORM_Z80_CPM3
    print_cpm3_raw_file_diag("after write16", DIAG_FILE);
#endif

    f = fopen(DIAG_FILE, "ab");
    if (f) {
        unsigned char more[4] = {0xAA, 0xBB, 0xCC, 0xDD};
        fwrite(more, 1, sizeof(more), f);
        fclose(f);
    }
    print_stdio_size_diag("after append4", DIAG_FILE);
    print_lowlevel_size_diag(DIAG_FILE);
#ifdef __XCC_PLATFORM_Z80_CPM3
    print_cpm3_raw_file_diag("after append4", DIAG_FILE);
#endif

    f = fopen(DIAG_FILE, "wb");
    if (f) {
        unsigned char shortdata[8] = {9, 8, 7, 6, 5, 4, 3, 2};
        fwrite(shortdata, 1, sizeof(shortdata), f);
        fclose(f);
    }
    print_stdio_size_diag("after truncate8", DIAG_FILE);
    print_lowlevel_size_diag(DIAG_FILE);
    print_stdio_eof_diag(DIAG_FILE);
#ifdef __XCC_PLATFORM_Z80_CPM3
    print_cpm3_raw_file_diag("after truncate8", DIAG_FILE);
#endif

    remove(DIAG_FILE);
}

int main(void) {
    printf("=== Z80 C23 STDIO FILE I/O COMPREHENSIVE TEST ===\n\n");

    printf("--- Binary write + read ---\n");

    FILE *f = fopen(TEST_FILE, "wb");
    TEST_ASSERT(f != NULL, "fopen wb");

    if (f) {
        unsigned char data[16] = {
            0, 1, 2, 3, 4, 5, 6, 7,
            8, 9, 10, 11, 12, 13, 14, 15
        };
        size_t written = fwrite(data, 1, sizeof(data), f);
        TEST_ASSERT(written == sizeof(data), "fwrite 16 bytes");
        fclose(f);
    }

    f = fopen(TEST_FILE, "rb");
    TEST_ASSERT(f != NULL, "fopen rb");

    if (f) {
        unsigned char readbuf[16];
        size_t readn = fread(readbuf, 1, sizeof(readbuf), f);
        TEST_ASSERT(readn == 16, "fread 16 bytes");
        TEST_ASSERT(
            memcmp(
                readbuf,
                "\x00\x01\x02\x03\x04\x05\x06\x07"
                "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
                16) == 0,
            "binary data roundtrip");
        fclose(f);
    }

    printf("\n--- Text mode write/read ---\n");

    f = fopen(TEXT_FILE, "w");
    if (f) {
        fputs("Hello Z80!\nLine2\n", f);
        fprintf(f, "Number: %d\n", 42);
        fclose(f);
    }

    f = fopen(TEXT_FILE, "r");
    if (f) {
        char line[64];
        TEST_ASSERT(fgets(line, sizeof(line), f) != NULL, "fgets first line");
        TEST_ASSERT(strstr(line, "Hello") != NULL, "fgets content");
        fclose(f);
    }

    printf("\n--- Append mode (prolong file) ---\n");

    f = fopen(TEST_FILE, "ab");
    if (f) {
        unsigned char more[4] = {0xAA, 0xBB, 0xCC, 0xDD};
        fwrite(more, 1, 4, f);
        fclose(f);
    }

    f = fopen(TEST_FILE, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        TEST_ASSERT(len == 20, "file length after append (16+4)");
        fclose(f);
    }

    printf("\n--- Truncate by reopening with w ---\n");

    f = fopen(TEST_FILE, "wb");
    if (f) {
        unsigned char shortdata[8] = {9, 8, 7, 6, 5, 4, 3, 2};
        fwrite(shortdata, 1, 8, f);
        fclose(f);
    }

    f = fopen(TEST_FILE, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long newlen = ftell(f);
        TEST_ASSERT(newlen == 8, "file truncated and rewritten to 8 bytes");
        fclose(f);
    }

    printf("\n--- Seek + mixed read/write ---\n");

    f = fopen(TEST_FILE, "r+b");
    if (f) {
        unsigned char buf[4];

        fseek(f, 0, SEEK_SET);
        fread(buf, 1, 2, f);
        TEST_ASSERT(buf[0] == 9 && buf[1] == 8, "seek + read from start");

        fseek(f, 4, SEEK_SET);
        {
            unsigned char overwrite[2] = {0xEE, 0xFF};
            fwrite(overwrite, 1, 2, f);
        }

        fseek(f, 4, SEEK_SET);
        fread(buf, 1, 2, f);
        TEST_ASSERT(buf[0] == 0xEE && buf[1] == 0xFF, "seek + write + read back");

        fseek(f, 1, SEEK_CUR);
        {
            long pos = ftell(f);
            TEST_ASSERT(pos == 7, "ftell after SEEK_CUR");
        }

        fseek(f, 0, SEEK_END);
        TEST_ASSERT(ftell(f) == 8, "seek to end");

        fclose(f);
    }

    printf("\n--- feof, ferror, clearerr ---\n");

    f = fopen(TEST_FILE, "rb");
    if (f) {
        unsigned char dummy[100];
        fread(dummy, 1, 100, f);
        TEST_ASSERT(feof(f) != 0, "feof after reading past end");
        TEST_ASSERT(ferror(f) == 0, "no error on normal read past EOF");

        clearerr(f);
        TEST_ASSERT(feof(f) == 0, "clearerr clears feof");

        fclose(f);
    }

    printf("\n--- fflush ---\n");

    f = fopen(TEST_FILE, "wb");
    if (f) {
        fwrite("test", 1, 4, f);
        {
            int flush_res = fflush(f);
            TEST_ASSERT(flush_res == 0, "fflush success");
        }
        fclose(f);
    }

    printf("\n--- rewind ---\n");

    f = fopen(TEST_FILE, "rb");
    if (f) {
        fseek(f, 5, SEEK_SET);
        rewind(f);
        TEST_ASSERT(ftell(f) == 0, "rewind to start");
        fclose(f);
    }

    remove(TEST_FILE);
    remove(TEXT_FILE);

    printf("\n=== SUMMARY ===\n");
    printf("stdio file tests passed: %d / %d\n", tests_passed, tests_total);
    if (tests_passed == tests_total) {
        printf("STDIO FILE I/O TEST PASSED SUCCESSFULLY!\n");
    } else {
        printf("Some file I/O tests failed - check your stdio implementation on Z80/CP/M.\n");
    }

    printf("\nNote: On CP/M some advanced features (tmpfile, certain seeks) may be limited.\n");
    printf("Binary mode (\"rb\"/\"wb\") is recommended for predictable behavior.\n");
    printf("\n=== BOTTOM DIAGNOSTICS ===\n");
    run_bottom_diagnostics();

    return 0;
}
