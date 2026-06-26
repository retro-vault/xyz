#include "xcc_exec_test.h"
#include "xcc_heap_test.h"

#include <stdlib.h>

#define ARENA_BYTES 192u

static heap_t heap;
static unsigned short arena_words[ARENA_BYTES / 2u];

int main(void) {
    unsigned char *base;
    unsigned char *limit;
    unsigned char *a;
    unsigned char *b;
    unsigned char *c;
    unsigned char *d;
    unsigned char *e;
    unsigned char *a_block;
    unsigned char *b_block;
    unsigned char *c_block;
    unsigned char *d_block;
    unsigned char *tail_block;
    unsigned char *tiny_tail;
    base = (unsigned char *)arena_words;
    limit = base + ARENA_BYTES;
    heap_init_arena(&heap, base, limit);

    XCC_CHECK_EQ_UINT_ID(1, xcc_heap_initial_payload(base, limit), 184u);
    XCC_CHECK_ID(2, xcc_heap_is_single_free_block(&heap));

    a = (unsigned char *)allocate(&heap, 18u);
    b = (unsigned char *)allocate(&heap, 22u);
    c = (unsigned char *)allocate(&heap, 14u);
    d = (unsigned char *)allocate(&heap, 30u);

    XCC_CHECK_ID(3, a != (unsigned char *)0);
    XCC_CHECK_ID(4, b != (unsigned char *)0);
    XCC_CHECK_ID(5, c != (unsigned char *)0);
    XCC_CHECK_ID(6, d != (unsigned char *)0);

    a_block = xcc_block_from_payload(a);
    b_block = xcc_block_from_payload(b);
    c_block = xcc_block_from_payload(c);
    d_block = xcc_block_from_payload(d);
    tail_block = xcc_block_next(d_block);

    XCC_CHECK_ID(7, a_block == base);
    XCC_CHECK_ID(8, b_block == a_block + XCC_HEAP_BLOCK_HDR_SIZE + 18u);
    XCC_CHECK_ID(9, c_block == b_block + XCC_HEAP_BLOCK_HDR_SIZE + 22u);
    XCC_CHECK_ID(10, d_block == c_block + XCC_HEAP_BLOCK_HDR_SIZE + 14u);
    XCC_CHECK_ID(11, tail_block == d_block + XCC_HEAP_BLOCK_HDR_SIZE + 30u);
    XCC_CHECK_EQ_UINT_ID(12, xcc_block_size(tail_block), 68u);
    XCC_CHECK_EQ_UINT_ID(13, xcc_block_free(tail_block), 1u);

    free(b);
    XCC_CHECK_EQ_UINT_ID(14, xcc_block_free(b_block), 1u);
    XCC_CHECK_ID(15, xcc_block_next(b_block) == c_block);
    XCC_CHECK_EQ_UINT_ID(16, xcc_block_size(tail_block), 68u);

    e = (unsigned char *)allocate(&heap, 10u);
    XCC_CHECK_ID(17, e == b);
    tiny_tail = xcc_block_next(b_block);
    XCC_CHECK_ID(18, tiny_tail == b_block + XCC_HEAP_BLOCK_HDR_SIZE + 10u);
    XCC_CHECK_EQ_UINT_ID(19, xcc_block_size(b_block), 10u);
    XCC_CHECK_EQ_UINT_ID(20, xcc_block_free(b_block), 0u);
    XCC_CHECK_EQ_UINT_ID(21, xcc_block_size(tiny_tail), 4u);
    XCC_CHECK_EQ_UINT_ID(22, xcc_block_free(tiny_tail), 1u);
    XCC_CHECK_ID(23, xcc_block_next(tiny_tail) == c_block);
    XCC_CHECK_EQ_UINT_ID(24, xcc_block_size(tail_block), 68u);

    free(c);
    XCC_CHECK_EQ_UINT_ID(25, xcc_block_size(tiny_tail), 26u);
    XCC_CHECK_EQ_UINT_ID(26, xcc_block_free(tiny_tail), 1u);
    XCC_CHECK_ID(27, xcc_block_next(tiny_tail) == d_block);
    XCC_CHECK_EQ_UINT_ID(28, xcc_block_size(tail_block), 68u);

    free(e);
    XCC_CHECK_EQ_UINT_ID(29, xcc_block_size(b_block), 44u);
    XCC_CHECK_EQ_UINT_ID(30, xcc_block_free(b_block), 1u);
    XCC_CHECK_ID(31, xcc_block_next(b_block) == d_block);
    XCC_CHECK_EQ_UINT_ID(32, xcc_block_size(tail_block), 68u);

    free(d);
    XCC_CHECK_EQ_UINT_ID(33, xcc_block_size(b_block), 158u);
    XCC_CHECK_EQ_UINT_ID(34, xcc_block_free(b_block), 1u);
    XCC_CHECK_ID(35, xcc_block_next(b_block) == (unsigned char *)0);

    deallocate(&heap, a);
    XCC_CHECK_ID(36, xcc_heap_is_single_free_block(&heap));
    return 0;
}
