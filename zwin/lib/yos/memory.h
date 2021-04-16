/*
 * memory.h
 *
 * memory management (mem_allocate, mem_free)
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 25.05.2012   tstih
 *
 */
#ifndef _MEMORY_H
#define _MEMORY_H

#include "yos.h"
#include "list.h"

#ifndef NONE
#define NONE 0
#endif

#define BLK_SIZE (sizeof(struct block_s) - sizeof(byte_t[1]))
#define MIN_CHUNK_SIZE 4

/* block status, use as bit operations */
#define NEW 0x00
#define ALLOCATED 0x01

typedef struct block_s
{
    list_header_t hdr;
    byte_t stat;
    addr_size_t size;
    byte_t data[0];
} block_t;

extern void *heap;

extern void mem_init(void *heap, addr_size_t size);
extern void *mem_allocate(void *heap, addr_size_t size, handle_t owner);
extern void *mem_free(void *heap, void *p);
extern void mem_copy(byte_t *src, byte_t *dest, addr_size_t count);



#endif
