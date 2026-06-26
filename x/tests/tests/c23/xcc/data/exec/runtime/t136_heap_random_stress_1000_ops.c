#include "xcc_exec_test.h"
#include "xcc_heap_test.h"

#include <stdlib.h>

#define ARENA_BYTES 4096u
#define SLOT_COUNT  96u
#define OP_COUNT    1000u

static heap_t heap;
static unsigned short arena_words[ARENA_BYTES / 2u];
static unsigned char *slots[SLOT_COUNT];
static unsigned char sizes[SLOT_COUNT];
static unsigned char fills[SLOT_COUNT];
static unsigned int rng_state = 0xACE1u;

static unsigned int next_rand16(void) {
    rng_state = (unsigned int)(rng_state * 25173u + 13849u);
    return rng_state;
}

static void fill_slot(unsigned int index) {
    unsigned int j;

    for (j = 0u; j < (unsigned int)sizes[index]; ++j) {
        slots[index][j] = fills[index];
    }
}

static int check_slot(unsigned int index) {
    unsigned int j;

    for (j = 0u; j < (unsigned int)sizes[index]; ++j) {
        if (slots[index][j] != fills[index]) return 0;
    }
    return 1;
}

int main(void) {
    unsigned char *base;
    unsigned char *limit;
    unsigned int successful_allocs;
    unsigned int successful_frees;
    unsigned int op;
    unsigned int slot;
    unsigned int size;
    unsigned int i;

    base = (unsigned char *)arena_words;
    limit = base + ARENA_BYTES;
    heap_init_arena(&heap, base, limit);

    successful_allocs = 0u;
    successful_frees = 0u;

    XCC_CHECK_ID(1, xcc_heap_is_single_free_block(&heap));

    for (op = 0u; op < OP_COUNT; ++op) {
        slot = next_rand16() % SLOT_COUNT;

        if (slots[slot] != (unsigned char *)0 && ((next_rand16() & 1u) != 0u)) {
            XCC_CHECK_ID(2, check_slot(slot));
            if ((op & 3u) == 0u) {
                deallocate(&heap, slots[slot]);
            } else {
                free(slots[slot]);
            }
            slots[slot] = (unsigned char *)0;
            sizes[slot] = 0u;
            fills[slot] = 0u;
            ++successful_frees;
        } else if (slots[slot] == (unsigned char *)0) {
            size = (next_rand16() % 48u) + 1u;
            slots[slot] = (unsigned char *)allocate(&heap, (size_t)size);
            if (slots[slot] != (unsigned char *)0) {
                sizes[slot] = (unsigned char)size;
                fills[slot] = (unsigned char)(0x31u + ((slot * 17u + op) & 0x4Fu));
                fill_slot(slot);
                ++successful_allocs;
            }
        } else {
            XCC_CHECK_ID(3, check_slot(slot));
        }
    }

    XCC_CHECK_ID(4, successful_allocs > 0u);
    XCC_CHECK_ID(5, successful_frees > 0u);

    for (i = 0u; i < SLOT_COUNT; ++i) {
        if (slots[i] != (unsigned char *)0) {
            XCC_CHECK_ID(6, check_slot(i));
            if ((i & 1u) == 0u) {
                free(slots[i]);
            } else {
                deallocate(&heap, slots[i]);
            }
            slots[i] = (unsigned char *)0;
        }
    }

    XCC_CHECK_ID(7, xcc_heap_is_single_free_block(&heap));
    return 0;
}
