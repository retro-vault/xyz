#include "xcc_exec_test.h"
#include "xcc_heap_test.h"

#include <stdlib.h>

#define ARENA_BYTES 128u

static heap_t heap;
static unsigned short arena_words[ARENA_BYTES / 2u];

int main(void) {
    unsigned char *base;
    unsigned char *limit;
    unsigned char *a;
    unsigned char *b;
    unsigned char *c;
    unsigned char *a_block;
    unsigned char *b_block;
    unsigned char *c_block;
    unsigned char *tail;

    base = (unsigned char *)arena_words;
    limit = base + ARENA_BYTES;
    heap_init_arena(&heap, base, limit);

    a = (unsigned char *)allocate(&heap, 16u);
    b = (unsigned char *)allocate(&heap, 20u);
    c = (unsigned char *)allocate(&heap, 12u);

    XCC_CHECK_ID(1, a != (unsigned char *)0);
    XCC_CHECK_ID(2, b != (unsigned char *)0);
    XCC_CHECK_ID(3, c != (unsigned char *)0);
    a_block = xcc_block_from_payload(a);
    b_block = xcc_block_from_payload(b);
    c_block = xcc_block_from_payload(c);
    tail = xcc_block_next(c_block);

    XCC_CHECK_ID(4, a_block == base);
    XCC_CHECK_ID(5, b_block == a_block + XCC_HEAP_BLOCK_HDR_SIZE + 16u);
    XCC_CHECK_ID(6, c_block == b_block + XCC_HEAP_BLOCK_HDR_SIZE + 20u);
    XCC_CHECK_ID(7, tail == c_block + XCC_HEAP_BLOCK_HDR_SIZE + 12u);
    XCC_CHECK_EQ_UINT_ID(8, xcc_block_size(a_block), 16u);
    XCC_CHECK_EQ_UINT_ID(9, xcc_block_size(b_block), 20u);
    XCC_CHECK_EQ_UINT_ID(10, xcc_block_size(c_block), 12u);
    XCC_CHECK_EQ_UINT_ID(11, xcc_block_size(tail), 48u);
    XCC_CHECK_ID(12, xcc_block_next(a_block) == b_block);
    XCC_CHECK_ID(13, xcc_block_next(b_block) == c_block);
    XCC_CHECK_ID(14, xcc_block_next(c_block) == tail);

    deallocate(&heap, b);
    XCC_CHECK_EQ_UINT_ID(15, xcc_block_free(b_block), 1u);
    XCC_CHECK_ID(16, xcc_block_next(a_block) == b_block);
    XCC_CHECK_ID(17, xcc_block_next(b_block) == c_block);
    XCC_CHECK_ID(18, xcc_block_next(c_block) == tail);

    free(c);
    XCC_CHECK_EQ_UINT_ID(19, xcc_block_size(b_block), 96u);
    XCC_CHECK_EQ_UINT_ID(20, xcc_block_free(b_block), 1u);
    XCC_CHECK_ID(21, xcc_block_next(b_block) == (unsigned char *)0);

    free(a);
    XCC_CHECK_ID(22, xcc_heap_is_single_free_block(&heap));
    XCC_CHECK_EQ_UINT_ID(23, xcc_heap_initial_payload(base, limit), 120u);
    return 0;
}
