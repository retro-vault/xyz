/*
 * mem.h
 *
 * memory allocation functions (malloc, free)
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#ifndef __MEM_H__
#define __MEM_H__

#include <sysobj.h>

#ifndef NONE
#define NONE 0
#endif

#define BLK_SIZE (sizeof(struct block_s) - sizeof(uint8_t[1]))
#define MIN_CHUNK_SIZE 4

/* block status, use as bit operations */
#define NEW 0x00
#define ALLOCATED 0x01

typedef struct block_s
{
    sysobj_t hdr;
    uint8_t stat;
    uint16_t size;
    uint8_t data[1];
} block_t;

extern void *_sys_heap;
extern void *_heap;

extern void mem_init(void *heap, uint16_t size);
extern void *mem_allocate(void *heap, uint16_t size, void *owner);
extern void *mem_free(void *heap, void *p);

#endif /* __MEM_H__ */