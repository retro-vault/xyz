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
#include <kernel/mem.h>

static void merge_with_next(block_t *b)
{
    block_t *bnext = b->hdr.next;
    b->size += (BLK_SIZE + bnext->size);
    b->hdr.next = bnext->hdr.next;
}

static void split(block_t *b, uint16_t size)
{
    block_t *nw;
    nw = (block_t *)((uint16_t)(b->data) + size);
    nw->hdr.next = b->hdr.next;
    nw->size = b->size - (size + BLK_SIZE);
    nw->hdr.owner = b->hdr.owner;
    nw->stat = b->stat;
    /* do not set owner and stat because
	   they'll be populated later */
    b->size = size;
    b->hdr.next = nw;
}


/*
 * initialize memory management
 * sample call:
 *      user_heap=0x8000;
 *      mem_init(user_heap, 0x8000);
 */
void mem_init(void *heap, uint16_t size)
{
    block_t *first = (block_t *)heap;
    first->hdr.next = NULL;
    first->size = size - BLK_SIZE;
    first->hdr.owner = NONE; /* no owner */
    first->stat = NEW;
}

/*
 * allocate memory block for owner
 */
void *mem_allocate(void *heap, uint16_t size, void *owner)
{
    block_t *b;
    uint8_t guard = 0;

    b = (block_t *)heap;
    while (b)
    {
        if (!(b->stat & ALLOCATED) && b->size >= size)
        {
            if (b->size - size > BLK_SIZE + MIN_CHUNK_SIZE)
                split(b, size);
            b->hdr.owner = owner;
            b->stat = ALLOCATED;
            return b->data;
        }
        b = (block_t *)b->hdr.next;
        if (++guard == 0) break;
    }

    return NULL;
}

/*
 * free memory block
 */
void *mem_free(void *heap, void *p)
{
    block_t *prev;
    block_t *b;
    uint8_t guard = 0;

    /* calculate block address from pointer */
    b = (block_t *)((uint16_t)p - BLK_SIZE);

    /* make sure it is a valid memory block by finding it */
    prev = NULL;
    {
        block_t *current = (block_t *)heap;
        while (current && current != b)
        {
            prev = current;
            current = (block_t *)current->hdr.next;
            if (++guard == 0) return NULL;
        }
        if (!current)
            return NULL;
    }

    b->hdr.owner = NONE; /* reclaim for the heap */
    b->stat = NEW;
    /*
             * merge 3 blocks if possible
             */
    if (prev && !(prev->stat & ALLOCATED))
    { /* try previous */
        merge_with_next(prev);
        b = prev;
    }
    if (b->hdr.next && !(((block_t *)(b->hdr.next))->stat & ALLOCATED)) /* try next */
        merge_with_next(b);

    return b->data;
}

/*
 * free all blocks owned by owner
 */
uint8_t mem_free_owner(void *heap, void *owner)
{
    block_t *b;
    uint8_t count = 0;
    uint8_t guard = 0;

    b = (block_t *)heap;
    while (b)
    {
        if ((b->stat & ALLOCATED) && b->hdr.owner == owner)
        {
            mem_free(heap, b->data);
            count++;
            b = (block_t *)heap;
            guard = 0;
            continue;
        }

        b = (block_t *)b->hdr.next;
        if (++guard == 0) break;
    }

    return count;
}
