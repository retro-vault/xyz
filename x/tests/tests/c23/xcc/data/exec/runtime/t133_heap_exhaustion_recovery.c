#include "xcc_exec_test.h"
#include "xcc_heap_test.h"

#include <stddef.h>
#include <stdlib.h>

#define ARENA_BYTES 512u
#define SLOT_COUNT  32u

static heap_t heap;
static unsigned short arena_words[ARENA_BYTES / 2u];
static unsigned char *slots[SLOT_COUNT];

int main(void) {
    unsigned char *base;
    unsigned char *limit;
    unsigned char *ptr;
    size_t near_full;
    unsigned int count;
    unsigned int i;

    base = (unsigned char *)arena_words;
    limit = base + ARENA_BYTES;
    heap_init_arena(&heap, base, limit);

    XCC_CHECK_ID(1, xcc_heap_head(&heap) == base);
    XCC_CHECK_ID(2, xcc_heap_base(&heap) == base);
    XCC_CHECK_ID(3, xcc_heap_limit(&heap) == limit);
    XCC_CHECK_EQ_UINT_ID(4, xcc_block_size(base), xcc_heap_initial_payload(base, limit));
    XCC_CHECK_EQ_UINT_ID(5, xcc_block_free(base), 1u);
    XCC_CHECK_ID(6, xcc_block_next(base) == (unsigned char *)0);
    XCC_CHECK_ID(7, xcc_block_heap(base) == &heap);
    XCC_CHECK_ID(8, xcc_heap_is_single_free_block(&heap));

    count = 0u;
    for (;;) {
        ptr = (unsigned char *)allocate(&heap, 24u);
        if (ptr == (unsigned char *)0) break;
        XCC_CHECK_ID(9, count < SLOT_COUNT);
        slots[count] = ptr;
        ptr[0] = (unsigned char)(count + 1u);
        ptr[23] = (unsigned char)(0xE0u + count);
        if (count != 0u) XCC_CHECK_ID(10, slots[count] > slots[count - 1u]);
        ++count;
    }

    XCC_CHECK_ID(11, count > 0u);
    XCC_CHECK_ID(12, allocate(&heap, 2u) == (void *)0);

    for (i = 0u; i < count; ++i) {
        XCC_CHECK_ID(13, slots[i] >= base + XCC_HEAP_BLOCK_HDR_SIZE);
        XCC_CHECK_ID(14, slots[i] < limit);
        XCC_CHECK_EQ_UINT_ID(15, slots[i][0], (unsigned int)(unsigned char)(i + 1u));
        XCC_CHECK_EQ_UINT_ID(16, slots[i][23], (unsigned int)(unsigned char)(0xE0u + i));
        free(slots[i]);
    }

    XCC_CHECK_ID(17, xcc_heap_is_single_free_block(&heap));

    near_full = xcc_heap_initial_payload(base, limit);
    ptr = (unsigned char *)allocate(&heap, near_full);
    XCC_CHECK_ID(18, ptr != (unsigned char *)0);
    free(ptr);

    XCC_CHECK_ID(19, xcc_heap_is_single_free_block(&heap));
    return 0;
}
