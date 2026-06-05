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
static unsigned long __libc_rand_state = 1UL;

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

static const char *__libc_skip_space(const char *text)
{
    while (*text != '\0' && isspace((unsigned char)*text)) {
        ++text;
    }
    return text;
}

static int __libc_digit_value(int ch)
{
    if (isdigit(ch)) {
        return ch - '0';
    }
    if (isalpha(ch)) {
        return toupper(ch) - 'A' + 10;
    }
    return -1;
}

static int __libc_prepare_base(const char **textp, int base)
{
    const char *text;
    int digit;

    text = *textp;
    if (base != 0 && (base < 2 || base > 36)) {
        return -1;
    }

    if (base == 0) {
        if (text[0] == '0') {
            digit = __libc_digit_value((unsigned char)text[2]);
            if ((text[1] == 'x' || text[1] == 'X') &&
                digit >= 0 &&
                digit < 16) {
                *textp = text + 2;
                return 16;
            }
            return 8;
        }
        return 10;
    }

    if (base == 16) {
        digit = __libc_digit_value((unsigned char)text[2]);
        if (text[0] == '0' &&
            (text[1] == 'x' || text[1] == 'X') &&
            digit >= 0 &&
            digit < 16) {
            *textp = text + 2;
        }
    }

    return base;
}

static unsigned long long __libc_parse_unsigned(const char *nptr,
                                                char **endptr,
                                                int base,
                                                unsigned long long cutoff,
                                                int *negative_out,
                                                int *overflow_out)
{
    const char *text;
    const char *digits;
    unsigned long long value;
    int negative;
    int any;
    int overflow;

    text = __libc_skip_space(nptr);
    negative = 0;
    if (*text == '+' || *text == '-') {
        negative = (*text == '-');
        ++text;
    }

    digits = text;
    base = __libc_prepare_base(&text, base);
    if (base < 0) {
        if (endptr != 0) {
            *endptr = (char *)nptr;
        }
        if (negative_out != 0) {
            *negative_out = negative;
        }
        if (overflow_out != 0) {
            *overflow_out = 0;
        }
        return 0ULL;
    }

    value = 0ULL;
    any = 0;
    overflow = 0;
    while (*text != '\0') {
        int digit;

        digit = __libc_digit_value((unsigned char)*text);
        if (digit < 0 || digit >= base) {
            break;
        }
        any = 1;
        if (!overflow) {
            if (value > (cutoff - (unsigned long long)digit) / (unsigned long long)base) {
                overflow = 1;
                value = cutoff;
            } else {
                value = value * (unsigned long long)base + (unsigned long long)digit;
            }
        }
        ++text;
    }

    if (!any) {
        text = digits;
        if (endptr != 0) {
            *endptr = (char *)nptr;
        }
        if (negative_out != 0) {
            *negative_out = negative;
        }
        if (overflow_out != 0) {
            *overflow_out = 0;
        }
        return 0ULL;
    }

    if (endptr != 0) {
        *endptr = (char *)text;
    }
    if (negative_out != 0) {
        *negative_out = negative;
    }
    if (overflow_out != 0) {
        *overflow_out = overflow;
    }
    return value;
}

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

int abs(int value)
{
    return value < 0 ? -value : value;
}

long labs(long value)
{
    return value < 0L ? -value : value;
}

long long llabs(long long value)
{
    return value < 0LL ? -value : value;
}

div_t div(int numer, int denom)
{
    div_t result;

    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

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

int atoi(const char *nptr)
{
    return (int)strtol(nptr, 0, 10);
}

long atol(const char *nptr)
{
    return strtol(nptr, 0, 10);
}

long long atoll(const char *nptr)
{
    return strtoll(nptr, 0, 10);
}

long strtol(const char *restrict nptr, char **restrict endptr, int base)
{
    unsigned long long value;
    unsigned long long cutoff;
    int negative;
    int overflow;

    cutoff = (unsigned long long)LONG_MAX;
    negative = 0;
    overflow = 0;
    value = __libc_parse_unsigned(nptr,
                                  endptr,
                                  base,
                                  cutoff + 1ULL,
                                  &negative,
                                  &overflow);
    if (endptr != 0 && *endptr == nptr) {
        return 0L;
    }

    if (overflow || (!negative && value > cutoff) || (negative && value > cutoff + 1ULL)) {
        errno = ERANGE;
        return negative ? LONG_MIN : LONG_MAX;
    }

    if (negative) {
        if (value == cutoff + 1ULL) {
            return LONG_MIN;
        }
        return -(long)value;
    }
    return (long)value;
}

unsigned long strtoul(const char *restrict nptr, char **restrict endptr, int base)
{
    unsigned long long value;
    int negative;
    int overflow;

    negative = 0;
    overflow = 0;
    value = __libc_parse_unsigned(nptr,
                                  endptr,
                                  base,
                                  (unsigned long long)ULONG_MAX,
                                  &negative,
                                  &overflow);
    if (endptr != 0 && *endptr == nptr) {
        return 0UL;
    }

    if (overflow || value > (unsigned long long)ULONG_MAX) {
        errno = ERANGE;
        return ULONG_MAX;
    }

    if (negative) {
        return 0UL - (unsigned long)value;
    }
    return (unsigned long)value;
}

long long strtoll(const char *restrict nptr, char **restrict endptr, int base)
{
    unsigned long long value;
    unsigned long long cutoff;
    int negative;
    int overflow;

    cutoff = (unsigned long long)LLONG_MAX;
    negative = 0;
    overflow = 0;
    value = __libc_parse_unsigned(nptr,
                                  endptr,
                                  base,
                                  cutoff + 1ULL,
                                  &negative,
                                  &overflow);
    if (endptr != 0 && *endptr == nptr) {
        return 0LL;
    }

    if (overflow || (!negative && value > cutoff) || (negative && value > cutoff + 1ULL)) {
        errno = ERANGE;
        return negative ? LLONG_MIN : LLONG_MAX;
    }

    if (negative) {
        if (value == cutoff + 1ULL) {
            return LLONG_MIN;
        }
        return -(long long)value;
    }
    return (long long)value;
}

unsigned long long strtoull(const char *restrict nptr, char **restrict endptr, int base)
{
    unsigned long long value;
    int negative;
    int overflow;

    negative = 0;
    overflow = 0;
    value = __libc_parse_unsigned(nptr,
                                  endptr,
                                  base,
                                  ULLONG_MAX,
                                  &negative,
                                  &overflow);
    if (endptr != 0 && *endptr == nptr) {
        return 0ULL;
    }

    if (overflow) {
        errno = ERANGE;
        return ULLONG_MAX;
    }

    if (negative) {
        return 0ULL - value;
    }
    return value;
}

int rand(void)
{
    __libc_rand_state = __libc_rand_state * 1103515245UL + 12345UL;
    return (int)((__libc_rand_state >> 16) & RAND_MAX);
}

void srand(unsigned int seed)
{
    __libc_rand_state = ((unsigned long)seed << 16) ^ (unsigned long)seed ^ 1UL;
}

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
