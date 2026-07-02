/*
 * Declares the heap allocator used for both system and process-owned
 * dynamic memory inside YOS.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __MEM_H__
#define __MEM_H__

#include <kernel/sysobj.h>

/*
 * Null owner sentinel used by allocator clients.
 */
#ifndef NONE
#define NONE 0
#endif

/*
 * Metadata bytes stored ahead of each allocation block payload.
 */
#define BLK_SIZE (sizeof(struct block_s) - sizeof(uint8_t[1]))
/*
 * Minimum payload size of one split heap chunk.
 */
#define MIN_CHUNK_SIZE 4

/*
 * Initial block state before any allocation.
 */
#define NEW 0x00
/*
 * Bit set when a block is currently allocated.
 */
#define ALLOCATED 0x01

/*
 * Heap allocation block header.
 */
typedef struct block_s
{
    sysobj_t hdr;
    uint8_t stat;
    uint16_t size;
    uint8_t data[1];
} block_t;

/*
 * System-owned allocator heap.
 */
extern void *__sys_heap;
#define _sys_heap __sys_heap
/*
 * Default process allocator heap.
 */
extern void *__heap;
#define _heap __heap

/*
 * Initialize one heap region.
 */
extern void mem_init(void *heap, uint16_t size);
/*
 * Allocate one block from the selected heap.
 */
extern void *mem_allocate(void *heap, uint16_t size, void *owner);
/*
 * Free one previously allocated block.
 */
extern void *mem_free(void *heap, void *p);
/*
 * Free all blocks owned by the supplied owner token.
 */
extern uint8_t mem_free_owner(void *heap, void *owner);

#endif /* __MEM_H__ */
