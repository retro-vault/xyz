/* mdrsave-yos.c
 *
 * Tiny standalone microdrive save/load check.
 *
 * Built multiple times with different TEST_NAME / TEST_LEN values so the
 * harness app itself stays as small as possible.
 */

#include <stdint.h>

#include <yos.h>
#include "../hello/crt0.h"

#ifndef TEST_NAME
#define TEST_NAME "t123"
#endif

#ifndef TEST_LEN
#define TEST_LEN 123
#endif

#ifndef TEST_SECTORS
#define TEST_SECTORS 1
#endif

#define TEST_DRIVE 1
#define DIR_MAX    8
#define BUF_LEN    777

int main(void) {
    yos_t *y = (yos_t*)query_service("yos");
    uint8_t *src;
    uint8_t *dst;
    mdr_file_t *files;
    uint8_t count;
    uint16_t i;
    int found = -1;

    if (!y) return 1;
    y->printf("%s %u\n", TEST_NAME, (unsigned)TEST_LEN);

    if (y->mdr_detect_drives() < TEST_DRIVE) {
        y->printf("NO MDR\n");
        return 1;
    }

    src = (uint8_t*)y->malloc(BUF_LEN);
    dst = (uint8_t*)y->malloc(BUF_LEN);
    files = (mdr_file_t*)y->malloc((unsigned int)(DIR_MAX * sizeof(mdr_file_t)));
    if (!src || !dst || !files) {
        y->printf("OOM\n");
        if (files) y->free(files);
        if (dst) y->free(dst);
        if (src) y->free(src);
        return 1;
    }

    {
        uint8_t v = 1;
        for (i = 0; i < BUF_LEN; ++i) {
            src[i] = v;
            dst[i] = 0;
            ++v;
            if (v == 12) v = 1;
        }
    }

    if (y->mdr_save(TEST_DRIVE, TEST_NAME, src, TEST_LEN) != 0) {
        y->printf("SAVE FAIL\n");
        goto fail;
    }

    count = y->mdr_dir(TEST_DRIVE, files, DIR_MAX);
    for (i = 0; i < count; ++i) {
        if (y->strcmp(files[i].name, TEST_NAME) == 0) {
            found = (int)i;
            break;
        }
    }
    if (found < 0 ||
        files[found].size != TEST_LEN ||
        files[found].sectors != TEST_SECTORS) {
        y->printf("DIR FAIL %u\n", count);
        goto fail;
    }

    if (y->mdr_load(TEST_DRIVE, TEST_NAME, dst) != 0) {
        y->printf("LOAD FAIL\n");
        goto fail;
    }

    for (i = 0; i < TEST_LEN; ++i) {
        if (src[i] != dst[i]) {
            y->printf("CMP FAIL %u\n", (unsigned)i);
            goto fail;
        }
    }

    y->printf("PASS\n");
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
