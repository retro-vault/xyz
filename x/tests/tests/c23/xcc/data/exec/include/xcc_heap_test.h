//
// Small heap-layout helpers shared by executable xcc allocator regressions.
// These helpers intentionally decode the allocator's on-target byte layout
// directly so tests can assert exact split/coalesce behavior.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#ifndef XCC_HEAP_TEST_H
#define XCC_HEAP_TEST_H

#include <stddef.h>
#include <stdlib.h>

#define XCC_HEAP_BLOCK_HDR_SIZE 8u

union xcc_heap_ptr_bits {
    unsigned int   u;
    unsigned char *p;
};

union xcc_heap_heap_bits {
    unsigned int u;
    heap_t      *p;
};

union xcc_heap_void_bits {
    const void  *p;
    unsigned int u;
};

static unsigned int xcc_heap_u16_at(const unsigned char *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned char *xcc_heap_decode_ptr(const unsigned char *p) {
    union xcc_heap_ptr_bits bits;
    bits.u = xcc_heap_u16_at(p);
    return bits.p;
}

static unsigned char *xcc_heap_ptr_from_u16(unsigned int u) {
    union xcc_heap_ptr_bits bits;
    bits.u = u;
    return bits.p;
}

static heap_t *xcc_heap_decode_heap(const unsigned char *p) {
    union xcc_heap_heap_bits bits;
    bits.u = xcc_heap_u16_at(p);
    return bits.p;
}

static unsigned int xcc_heap_ptr_value(const void *p) {
    union xcc_heap_void_bits bits;
    bits.p = p;
    return bits.u;
}

static unsigned char *xcc_heap_head(heap_t *heap) {
    return xcc_heap_decode_ptr((unsigned char *)heap);
}

static unsigned char *xcc_heap_base(heap_t *heap) {
    return xcc_heap_decode_ptr((unsigned char *)heap + 2u);
}

static unsigned char *xcc_heap_limit(heap_t *heap) {
    return xcc_heap_decode_ptr((unsigned char *)heap + 4u);
}

static unsigned char *xcc_block_from_payload(void *ptr) {
    return (unsigned char *)ptr - XCC_HEAP_BLOCK_HDR_SIZE;
}

static unsigned char *xcc_block_payload(const unsigned char *block) {
    return (unsigned char *)block + XCC_HEAP_BLOCK_HDR_SIZE;
}

static unsigned int xcc_block_size(const unsigned char *block) {
    return xcc_heap_u16_at(block);
}

static unsigned int xcc_block_free(const unsigned char *block) {
    return xcc_heap_u16_at(block + 2u);
}

static unsigned char *xcc_block_next(const unsigned char *block) {
    return xcc_heap_decode_ptr(block + 4u);
}

static heap_t *xcc_block_heap(const unsigned char *block) {
    return xcc_heap_decode_heap(block + 6u);
}

static unsigned int xcc_heap_initial_payload(const unsigned char *base,
                                             const unsigned char *limit) {
    return (unsigned int)(limit - base - XCC_HEAP_BLOCK_HDR_SIZE);
}

static __attribute__((noinline)) unsigned int xcc_heap_count_blocks(heap_t *heap) {
    unsigned char *block;
    volatile unsigned int block_u;
    unsigned int count;

    block_u = xcc_heap_u16_at((unsigned char *)heap);
    count = 0u;
    while (block_u != 0u) {
        block = xcc_heap_ptr_from_u16(block_u);
        ++count;
        if (count > 2048u) return 0u;
        block_u = xcc_heap_u16_at(block + 4u);
    }
    return count;
}

static __attribute__((noinline)) unsigned int xcc_heap_total_free_bytes(heap_t *heap) {
    unsigned char *block;
    volatile unsigned int block_u;
    unsigned int total;

    block_u = xcc_heap_u16_at((unsigned char *)heap);
    total = 0u;
    while (block_u != 0u) {
        block = xcc_heap_ptr_from_u16(block_u);
        if (xcc_block_free(block) != 0u) total += xcc_block_size(block);
        block_u = xcc_heap_u16_at(block + 4u);
    }
    return total;
}

static __attribute__((noinline)) int xcc_heap_validate_chain(heap_t *heap) {
    unsigned char *block;
    unsigned int base_u;
    volatile unsigned int block_u;
    volatile unsigned int end_u;
    unsigned int heap_u;
    unsigned int limit_u;
    volatile unsigned int next_u;
    volatile unsigned int size_u;
    unsigned int count;

    block_u = xcc_heap_u16_at((unsigned char *)heap);
    base_u = xcc_heap_u16_at((unsigned char *)heap + 2u);
    limit_u = xcc_heap_u16_at((unsigned char *)heap + 4u);
    heap_u = xcc_heap_ptr_value(heap);

    if (base_u == 0u) return 0;
    if (limit_u == 0u) return 0;
    if (block_u != base_u) return 0;

    count = 0u;
    while (block_u != 0u) {
        block = xcc_heap_ptr_from_u16(block_u);
        ++count;
        if (count > 2048u) return 0;
        size_u = xcc_heap_u16_at(block);
        if ((size_u & 1u) != 0u) return 0;
        if (xcc_heap_u16_at(block + 6u) != heap_u) return 0;

        end_u = (unsigned int)(block_u + XCC_HEAP_BLOCK_HDR_SIZE + size_u);
        next_u = xcc_heap_u16_at(block + 4u);
        if (next_u != 0u) {
            if (next_u != end_u) return 0;
        } else if (end_u != limit_u) {
            return 0;
        }

        block_u = next_u;
    }

    return count != 0u;
}

static __attribute__((noinline)) int xcc_heap_is_single_free_block(heap_t *heap) {
    unsigned char *head;
    unsigned char *base;
    unsigned char *limit;
    unsigned int head_u;
    unsigned int base_u;

    head = xcc_heap_head(heap);
    base = xcc_heap_base(heap);
    limit = xcc_heap_limit(heap);
    head_u = xcc_heap_u16_at((unsigned char *)heap);
    base_u = xcc_heap_u16_at((unsigned char *)heap + 2u);

    if (head_u != base_u) return 0;
    if (xcc_block_size(head) != xcc_heap_initial_payload(base, limit)) return 0;
    if (xcc_block_free(head) != 1u) return 0;
    if (xcc_block_next(head) != (unsigned char *)0) return 0;
    if (xcc_block_heap(head) != heap) return 0;
    return 1;
}

#endif
