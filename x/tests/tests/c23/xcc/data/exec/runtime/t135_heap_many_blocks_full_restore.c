#include "xcc_exec_test.h"
#include "xcc_heap_test.h"

#include <stdlib.h>

#define ARENA_BYTES 1024u
#define SLOT_COUNT  32u

static heap_t heap;
static unsigned short arena_words[ARENA_BYTES / 2u];
static unsigned char *slots[SLOT_COUNT];
static unsigned char sizes[SLOT_COUNT];
static unsigned char fills[SLOT_COUNT];

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
    unsigned char *initial_head;
    unsigned char *initial_base;
    unsigned char *initial_limit;
    unsigned int initial_free;
    unsigned int i;

    base = (unsigned char *)arena_words;
    limit = base + ARENA_BYTES;
    heap_init_arena(&heap, base, limit);

    initial_head = xcc_heap_head(&heap);
    initial_base = xcc_heap_base(&heap);
    initial_limit = xcc_heap_limit(&heap);
    initial_free = xcc_heap_initial_payload(base, limit);

    XCC_CHECK_ID(1, xcc_heap_is_single_free_block(&heap));

    for (i = 0u; i < SLOT_COUNT; ++i) {
        sizes[i] = (unsigned char)(6u + (i % 5u) * 6u);
        fills[i] = (unsigned char)(0x40u + i);
        slots[i] = (unsigned char *)allocate(&heap, (size_t)sizes[i]);
        XCC_CHECK_ID(2, slots[i] != (unsigned char *)0);
        fill_slot(i);
    }

    for (i = 0u; i < SLOT_COUNT; ++i) {
        XCC_CHECK_ID(3, check_slot(i));
    }

    for (i = 1u; i < SLOT_COUNT; i += 2u) {
        XCC_CHECK_ID(4, check_slot(i));
        free(slots[i]);
        slots[i] = (unsigned char *)0;
    }

    for (i = 0u; i < SLOT_COUNT; i += 2u) {
        XCC_CHECK_ID(5, check_slot(i));
    }

    i = SLOT_COUNT;
    while (i != 0u) {
        --i;
        if (slots[i] != (unsigned char *)0) {
            XCC_CHECK_ID(6, check_slot(i));
            deallocate(&heap, slots[i]);
            slots[i] = (unsigned char *)0;
        }
    }

    XCC_CHECK_ID(7, xcc_heap_is_single_free_block(&heap));
    XCC_CHECK_ID(8, xcc_heap_head(&heap) == initial_head);
    XCC_CHECK_ID(9, xcc_heap_base(&heap) == initial_base);
    XCC_CHECK_ID(10, xcc_heap_limit(&heap) == initial_limit);
    XCC_CHECK_EQ_UINT_ID(11, xcc_block_size(initial_head), initial_free);
    return 0;
}
