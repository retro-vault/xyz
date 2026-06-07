/*
 * stdlib.c
 *
 * Freestanding general-utility runtime for the xcc Z80 libc.
 *
 * This file provides a compact heap allocator, integer conversion helpers,
 * search/sort utilities, and process-termination hooks suitable for the
 * target's flat 16-bit address space.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define __LIBC_HEAP_SIZE          8192U
#define __LIBC_ATEXIT_SLOTS       8U
#define __LIBC_QUICK_EXIT_SLOTS   8U

typedef struct __libc_heap_block {
    size_t                    size;
    int                       free;
    struct __libc_heap_block *next;
} __libc_heap_block;

static union {
    unsigned short align;
    unsigned char  bytes[__LIBC_HEAP_SIZE];
} __libc_heap_storage;

static __libc_heap_block *__libc_heap_head;
static int __libc_heap_ready;

static void (*__libc_atexit_handlers[__LIBC_ATEXIT_SLOTS])(void);
static size_t __libc_atexit_count;
static void (*__libc_quick_exit_handlers[__LIBC_QUICK_EXIT_SLOTS])(void);
static size_t __libc_quick_exit_count;

volatile int __libc_exit_status;
volatile int __libc_exit_kind;

static size_t __libc_align_size(size_t size)
{
    return (size + 1U) & ~(size_t)1U;
}

static void __libc_heap_init(void)
{
    if (__libc_heap_ready) {
        return;
    }

    __libc_heap_head = (__libc_heap_block *)__libc_heap_storage.bytes;
    __libc_heap_head->size = __LIBC_HEAP_SIZE - sizeof(__libc_heap_block);
    __libc_heap_head->free = 1;
    __libc_heap_head->next = 0;
    __libc_heap_ready = 1;
}

static void __libc_heap_split(__libc_heap_block *block, size_t size)
{
    __libc_heap_block *tail;
    unsigned char *payload;

    if (block->size <= size + sizeof(__libc_heap_block) + 1U) {
        return;
    }

    payload = (unsigned char *)(block + 1);
    tail = (__libc_heap_block *)(payload + size);
    tail->size = block->size - size - sizeof(__libc_heap_block);
    tail->free = 1;
    tail->next = block->next;

    block->size = size;
    block->next = tail;
}

static void __libc_heap_coalesce(void)
{
    __libc_heap_block *block;

    block = __libc_heap_head;
    while (block != 0 && block->next != 0) {
        unsigned char *block_end;

        block_end = (unsigned char *)(block + 1) + block->size;
        if (block->free &&
            block->next->free &&
            block_end == (unsigned char *)block->next) {
            block->size += sizeof(__libc_heap_block) + block->next->size;
            block->next = block->next->next;
        } else {
            block = block->next;
        }
    }
}

static __libc_heap_block *__libc_ptr_to_block(void *ptr)
{
    if (ptr == 0) {
        return 0;
    }
    return ((__libc_heap_block *)ptr) - 1;
}

/* The string->number parser lives in strtox_core.s (assembly). */

static void __libc_swap_bytes(unsigned char *lhs, unsigned char *rhs, size_t size)
{
    while (size != 0U) {
        unsigned char tmp;

        tmp = *lhs;
        *lhs = *rhs;
        *rhs = tmp;
        ++lhs;
        ++rhs;
        --size;
    }
}

static void __libc_run_handlers(void (**handlers)(void), size_t count)
{
    while (count != 0U) {
        --count;
        handlers[count]();
    }
}

void abort(void)
{
    __libc_exit_kind = 2;
    __libc_exit_status = EXIT_FAILURE;
    for (;;) {
    }
}

int atexit(void (*func)(void))
{
    if (func == 0 || __libc_atexit_count >= __LIBC_ATEXIT_SLOTS) {
        return 1;
    }
    __libc_atexit_handlers[__libc_atexit_count++] = func;
    return 0;
}

void exit(int status)
{
    __libc_run_handlers(__libc_atexit_handlers, __libc_atexit_count);
    __libc_exit_kind = 1;
    __libc_exit_status = status;
    for (;;) {
    }
}

void _Exit(int status)
{
    __libc_exit_kind = 3;
    __libc_exit_status = status;
    for (;;) {
    }
}

int at_quick_exit(void (*func)(void))
{
    if (func == 0 || __libc_quick_exit_count >= __LIBC_QUICK_EXIT_SLOTS) {
        return 1;
    }
    __libc_quick_exit_handlers[__libc_quick_exit_count++] = func;
    return 0;
}

void quick_exit(int status)
{
    __libc_run_handlers(__libc_quick_exit_handlers, __libc_quick_exit_count);
    __libc_exit_kind = 4;
    __libc_exit_status = status;
    for (;;) {
    }
}

void *malloc(size_t size)
{
    __libc_heap_block *block;
    size_t want;

    if (size == 0U) {
        return 0;
    }

    __libc_heap_init();
    want = __libc_align_size(size);

    block = __libc_heap_head;
    while (block != 0) {
        if (block->free && block->size >= want) {
            __libc_heap_split(block, want);
            block->free = 0;
            return (void *)(block + 1);
        }
        block = block->next;
    }

    return 0;
}

void *calloc(size_t count, size_t size)
{
    void *memory;
    size_t total;

    if (count == 0U || size == 0U) {
        return malloc(0U);
    }
    if (count > SIZE_MAX / size) {
        return 0;
    }

    total = count * size;
    memory = malloc(total);
    if (memory != 0) {
        memset(memory, 0, total);
    }
    return memory;
}

void *realloc(void *ptr, size_t size)
{
    __libc_heap_block *block;
    void *replacement;
    size_t copy_size;

    if (ptr == 0) {
        return malloc(size);
    }
    if (size == 0U) {
        free(ptr);
        return 0;
    }

    block = __libc_ptr_to_block(ptr);
    size = __libc_align_size(size);

    if (block->size >= size) {
        __libc_heap_split(block, size);
        return ptr;
    }

    if (block->next != 0 &&
        block->next->free &&
        ((unsigned char *)(block + 1) + block->size) == (unsigned char *)block->next &&
        block->size + sizeof(__libc_heap_block) + block->next->size >= size) {
        block->size += sizeof(__libc_heap_block) + block->next->size;
        block->next = block->next->next;
        __libc_heap_split(block, size);
        return ptr;
    }

    replacement = malloc(size);
    if (replacement == 0) {
        return 0;
    }

    copy_size = block->size < size ? block->size : size;
    memcpy(replacement, ptr, copy_size);
    free(ptr);
    return replacement;
}

void free(void *ptr)
{
    __libc_heap_block *block;

    if (ptr == 0) {
        return;
    }

    block = __libc_ptr_to_block(ptr);
    block->free = 1;
    __libc_heap_coalesce();
}

/* abs, labs, llabs, and div are implemented in assembly:
 * see abs.s, labs.s, llabs.s, div.s in this directory. */

ldiv_t ldiv(long numer, long denom)
{
    ldiv_t result;

    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

lldiv_t lldiv(long long numer, long long denom)
{
    lldiv_t result;

    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

/* atoi, atol, atoll, strtol, strtoul, strtoll, strtoull are implemented
 * in assembly: see atoi.s, atol.s, atoll.s, strtol.s, strtoul.s,
 * strtoll.s, strtoull.s and the shared strtox_core.s. */

void *bsearch(const void *key,
              const void *base,
              size_t count,
              size_t size,
              int (*compar)(const void *, const void *))
{
    size_t low;
    size_t high;
    const unsigned char *bytes;

    if (key == 0 || base == 0 || compar == 0 || size == 0U) {
        return 0;
    }

    low = 0U;
    high = count;
    bytes = (const unsigned char *)base;

    while (low < high) {
        size_t mid;
        const void *entry;
        int order;

        mid = low + (high - low) / 2U;
        entry = bytes + mid * size;
        order = compar(key, entry);
        if (order == 0) {
            return (void *)entry;
        }
        if (order < 0) {
            high = mid;
        } else {
            low = mid + 1U;
        }
    }

    return 0;
}

void qsort(void *base,
           size_t count,
           size_t size,
           int (*compar)(const void *, const void *))
{
    unsigned char *bytes;
    size_t i;

    if (base == 0 || compar == 0 || size == 0U || count < 2U) {
        return;
    }

    bytes = (unsigned char *)base;
    i = 1U;
    while (i < count) {
        size_t j;

        j = i;
        while (j > 0U) {
            unsigned char *lhs;
            unsigned char *rhs;

            lhs = bytes + (j - 1U) * size;
            rhs = bytes + j * size;
            if (compar(lhs, rhs) <= 0) {
                break;
            }
            __libc_swap_bytes(lhs, rhs, size);
            --j;
        }
        ++i;
    }
}
