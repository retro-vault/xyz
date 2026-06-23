/* mdrtst-yos.c
 *
 * Standalone microdrive save/load/dir self-test app.
 *
 * This lives outside the ROM shell so we can exercise save safely on a
 * dedicated cartridge without touching the working command loader.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 Tomaz Stih
 */

#include <stdint.h>

#include <yos.h>
#include "../../../x/tests/hello/crt0.h"

#define TEST_DRIVE      1
#define MAX_FILES       32
#define SMALL_LEN       123
#define EDGE_LEN        512
#define MULTI_LEN       777
#define PATTERN_SPAN    11
#define NAME_BUF_LEN    (MDR_NAME_LEN + 1)

static void fill_pattern(uint8_t *buf, uint16_t len) {
    uint16_t i;
    uint8_t value = 1;
    for (i = 0; i < len; ++i) {
        buf[i] = value;
        ++value;
        if (value > PATTERN_SPAN) {
            value = 1;
        }
    }
}

static void fill_value(uint8_t *buf, uint16_t len, uint8_t value) {
    uint16_t i;
    for (i = 0; i < len; ++i) {
        buf[i] = value;
    }
}

static uint8_t compare_bytes(uint8_t *lhs, uint8_t *rhs, uint16_t len) {
    uint16_t i;
    for (i = 0; i < len; ++i) {
        if (lhs[i] != rhs[i]) {
            return 0;
        }
    }
    return 1;
}

static uint8_t streq(yos_t *y, const char *lhs, const char *rhs) {
    return (uint8_t)(y->strcmp(lhs, rhs) == 0);
}

static void hex2(uint8_t value, char *out) {
    uint8_t hi = (uint8_t)((value >> 4) & 0x0f);
    uint8_t lo = (uint8_t)(value & 0x0f);
    out[0] = (char)((hi < 10) ? ('0' + hi) : ('A' + (hi - 10)));
    out[1] = (char)((lo < 10) ? ('0' + lo) : ('A' + (lo - 10)));
}

static void make_names(uint8_t slot, char *small, char *edge, char *multi) {
    small[0] = 's';
    edge[0]  = 's';
    multi[0] = 's';
    hex2(slot, small + 1);
    hex2(slot, edge + 1);
    hex2(slot, multi + 1);

    small[3] = 's';
    small[4] = 'm';
    small[5] = 'a';
    small[6] = 'l';
    small[7] = 'l';
    small[8] = 0;

    edge[3] = 'e';
    edge[4] = 'd';
    edge[5] = 'g';
    edge[6] = 'e';
    edge[7] = 0;

    multi[3] = 'm';
    multi[4] = 'u';
    multi[5] = 'l';
    multi[6] = 't';
    multi[7] = 'i';
    multi[8] = 0;
}

static uint8_t name_in_use(yos_t *y, mdr_file_t *files, uint8_t count, char *name) {
    uint8_t i;
    for (i = 0; i < count; ++i) {
        if (streq(y, files[i].name, name)) {
            return 1;
        }
    }
    return 0;
}

static uint8_t choose_slot(yos_t *y, mdr_file_t *files, uint8_t count,
                           char *small, char *edge, char *multi) {
    uint16_t slot;
    for (slot = 0; slot < 256; ++slot) {
        make_names((uint8_t)slot, small, edge, multi);
        if (!name_in_use(y, files, count, small) &&
            !name_in_use(y, files, count, edge) &&
            !name_in_use(y, files, count, multi)) {
            return 1;
        }
    }
    return 0;
}

static int find_file(yos_t *y, mdr_file_t *files, uint8_t count, char *name) {
    uint8_t i;
    for (i = 0; i < count; ++i) {
        if (streq(y, files[i].name, name)) {
            return (int)i;
        }
    }
    return -1;
}

static void print_dir(yos_t *y, mdr_file_t *files, uint8_t count) {
    uint8_t i;
    y->printf("DIR COUNT=%u\n", count);
    for (i = 0; i < count && i < 8; ++i) {
        y->printf("%u:%s %u %u\n",
                  i,
                  files[i].name,
                  files[i].sectors,
                  files[i].size);
    }
}

static uint8_t save_one(yos_t *y, uint8_t drive, char *name,
                        uint8_t *src, uint16_t len) {
    y->printf("%s %u\n", name, len);
    if (y->mdr_save(drive, name, src, len) != 0) {
        y->printf("SAVE FAIL %s\n", name);
        return 0;
    }
    y->printf("SAVE OK %s\n", name);
    return 1;
}

static uint8_t check_dir_entry(yos_t *y, mdr_file_t *files, uint8_t count,
                               char *name, uint8_t sectors, uint16_t size) {
    int idx = find_file(y, files, count, name);
    if (idx < 0) {
        return 0;
    }
    return (uint8_t)(files[idx].sectors == sectors && files[idx].size == size);
}

static uint8_t load_and_check(yos_t *y, uint8_t drive, char *name,
                              uint8_t *src, uint8_t *dst,
                              uint16_t len) {
    fill_value(dst, MULTI_LEN, 0);
    if (y->mdr_load(drive, name, dst) != 0) {
        y->printf("LOAD FAIL %s\n", name);
        return 0;
    }
    if (!compare_bytes(src, dst, len)) {
        y->printf("VERIFY FAIL %s\n", name);
        return 0;
    }
    y->printf("LOAD OK %s\n", name);
    return 1;
}

int main(void) {
    yos_t *y = (yos_t*)query_service("yos");
    uint8_t drives;
    uint8_t dir_count;
    uint8_t *src;
    uint8_t *dst;
    mdr_file_t *files;
    char small[NAME_BUF_LEN];
    char edge[NAME_BUF_LEN];
    char multi[NAME_BUF_LEN];

    if (!y) {
        return 1;
    }

    y->printf("MICRODRIVE SAVE TEST\n");
    y->printf("DRIVE %u\n", TEST_DRIVE);

    drives = y->mdr_detect_drives();
    if (drives < TEST_DRIVE) {
        y->printf("NO MICRODRIVE DETECTED\n");
        return 1;
    }

    src = (uint8_t*)y->malloc(MULTI_LEN);
    dst = (uint8_t*)y->malloc(MULTI_LEN);
    files = (mdr_file_t*)y->malloc((unsigned int)(MAX_FILES * sizeof(mdr_file_t)));
    if (!src || !dst || !files) {
        y->printf("OUT OF MEMORY\n");
        if (files) y->free(files);
        if (dst) y->free(dst);
        if (src) y->free(src);
        return 1;
    }

    fill_pattern(src, MULTI_LEN);

    dir_count = y->mdr_dir(TEST_DRIVE, files, MAX_FILES);
    if (!choose_slot(y, files, dir_count, small, edge, multi)) {
        y->printf("NO FREE TEST NAMES\n");
        y->free(files);
        y->free(dst);
        y->free(src);
        return 1;
    }

    y->printf("PATTERN 1..11\n");
    y->printf("FILES %s %s %s\n", small, edge, multi);

    if (!save_one(y, TEST_DRIVE, small, src, SMALL_LEN)) goto fail;
    if (!save_one(y, TEST_DRIVE, edge, src, EDGE_LEN)) goto fail;
    if (!save_one(y, TEST_DRIVE, multi, src, MULTI_LEN)) goto fail;

    dir_count = y->mdr_dir(TEST_DRIVE, files, MAX_FILES);
    if (!check_dir_entry(y, files, dir_count, small, 1, SMALL_LEN) ||
        !check_dir_entry(y, files, dir_count, edge, 1, EDGE_LEN) ||
        !check_dir_entry(y, files, dir_count, multi, 2, MULTI_LEN)) {
        y->printf("DIR FAIL\n");
        print_dir(y, files, dir_count);
        goto fail;
    }
    y->printf("DIR OK\n");

    if (!load_and_check(y, TEST_DRIVE, small, src, dst, SMALL_LEN)) goto fail;
    if (!load_and_check(y, TEST_DRIVE, edge, src, dst, EDGE_LEN)) goto fail;
    if (!load_and_check(y, TEST_DRIVE, multi, src, dst, MULTI_LEN)) goto fail;

    y->printf("MDR TEST PASS\n");
    y->free(files);
    y->free(dst);
    y->free(src);
    return 0;

fail:
    y->free(files);
    y->free(dst);
    y->free(src);
    return 1;
}
