#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <yos.h>

extern yos_t *yos_api_table;

#define YOS_ALLOC_OVERHEAD 6u
#define YOS_ALLOC_CURRENT_BANK 0x8000u
#define YOS_NEAR_ALLOC_MAX 16370u
#define ALIGNED_MAGIC_LO 0x6cu
#define ALIGNED_MAGIC_HI 0xa1u

static void set_nomem(void)
{
    errno = ENOMEM;
}

void *malloc(size_t size)
{
    unsigned char *raw;
    union {
        yos_user_ptr_t pointer;
        unsigned char bytes[3];
    } allocated;
    size_t total;

    if (size == 0)
        return NULL;
    if (!yos_api_table || !yos_api_table->allocate_memory ||
            size > YOS_NEAR_ALLOC_MAX) {
        set_nomem();
        return NULL;
    }

    total = (size + 7u) & ~1u;
    /* A standard C pointer is 16 bits. Ask YOS to allocate only in the bank
     * currently executing this process or library, then deliberately narrow
     * the returned far pointer. All-bank allocations use allocate_memory
     * directly and retain yos_user_ptr_t's bank byte. */
    allocated.pointer = yos_api_table->allocate_memory(
        total | YOS_ALLOC_CURRENT_BANK);
    raw = (unsigned char *)allocated.pointer;
    if (!raw) {
        set_nomem();
        return NULL;
    }
    /* Keep the aligned_alloc tag probe inside every allocation. */
    raw[0] = allocated.bytes[0];
    raw[2] = 0;
    raw[3] = 0;
    raw[4] = (unsigned char)size;
    raw[5] = (unsigned char)(size >> 8);
    return raw + YOS_ALLOC_OVERHEAD;
}

static unsigned char *allocation_base(void *pointer)
{
    unsigned char *user = (unsigned char *)pointer;

    if (user[-4] == ALIGNED_MAGIC_LO && user[-3] == ALIGNED_MAGIC_HI)
        user = (unsigned char *)(uintptr_t)
            ((uint16_t)user[-2] | ((uint16_t)user[-1] << 8));
    return user - YOS_ALLOC_OVERHEAD;
}

static yos_user_ptr_t allocation_far_pointer(void *pointer)
{
    union {
        yos_user_ptr_t pointer;
        unsigned char bytes[3];
    } result;
    unsigned char *base = (unsigned char *)pointer;
    uintptr_t address = (uintptr_t)base;

    result.bytes[0] = base[0];
    result.bytes[1] = (unsigned char)address;
    result.bytes[2] = (unsigned char)(address >> 8);
    return result.pointer;
}

void free(void *pointer)
{
    if (!pointer || !yos_api_table || !yos_api_table->free_memory)
        return;
    yos_api_table->free_memory(
        allocation_far_pointer(allocation_base(pointer)));
}

void *realloc(void *pointer, size_t size)
{
    unsigned char *user = (unsigned char *)pointer;
    unsigned char *replacement;
    size_t old_size;
    size_t copy_size;

    if (!pointer)
        return malloc(size);
    if (size == 0) {
        free(pointer);
        return NULL;
    }

    if (user[-4] == ALIGNED_MAGIC_LO && user[-3] == ALIGNED_MAGIC_HI)
        old_size = (size_t)user[-6] | ((size_t)user[-5] << 8);
    else
        old_size = (size_t)user[-2] | ((size_t)user[-1] << 8);

    replacement = (unsigned char *)malloc(size);
    if (!replacement)
        return NULL;
    copy_size = old_size < size ? old_size : size;
    memcpy(replacement, pointer, copy_size);
    free(pointer);
    return replacement;
}
