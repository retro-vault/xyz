/* mdrstep-yos.c
 *
 * Tiny single-purpose microdrive test app.
 */

#include <stdint.h>

#include <yos.h>
#include "../../../x/tests/hello/crt0.h"

#ifndef TEST_NAME
#define TEST_NAME "t123"
#endif

#ifndef TEST_LEN
#define TEST_LEN 123
#endif

#ifndef TEST_MODE_SAVE
#define TEST_MODE_SAVE 1
#endif

static uint8_t buf[777];
static uint8_t expect[777];

static void fill_pattern(uint8_t *dst) {
    uint16_t i;
    uint8_t v = 1;
    for (i = 0; i < 777; ++i) {
        dst[i] = v;
        ++v;
        if (v == 12) v = 1;
    }
}

int main(void) {
    yos_t *y = (yos_t*)query_service("yos");
    uint16_t i;

    if (!y) return 1;
    if (y->mdr_detect_drives() < 1) {
        y->printf("NO MDR\n");
        return 1;
    }

    fill_pattern(expect);

#if TEST_MODE_SAVE
    y->printf("S %s %u\n", TEST_NAME, (unsigned)TEST_LEN);
    if (y->mdr_save(1, TEST_NAME, expect, TEST_LEN) == 0) {
        y->printf("OK\n");
        return 0;
    }
    y->printf("FAIL\n");
    return 1;
#else
    y->printf("L %s %u\n", TEST_NAME, (unsigned)TEST_LEN);
    for (i = 0; i < 777; ++i) buf[i] = 0;
    if (y->mdr_load(1, TEST_NAME, buf) != 0) {
        y->printf("LOAD\n");
        return 1;
    }
    for (i = 0; i < TEST_LEN; ++i) {
        if (buf[i] != expect[i]) {
            y->printf("CMP %u\n", (unsigned)i);
            return 1;
        }
    }
    y->printf("OK\n");
    return 0;
#endif
}
