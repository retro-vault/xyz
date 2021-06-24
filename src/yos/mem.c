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
#include <mem.h>


/*
 * ---------- private (helpers) ----------
 */

uint8_t match_free_block(list_item_t *p, uint16_t size)
{
    block_t *b = (block_t *)p;
    return !(b->stat & ALLOCATED) && b->size >= size;
}

void merge_with_next(block_t *b)
{
    block_t *bnext = b->hdr.next;
    b->size += (BLK_SIZE + bnext->size);
    b->hdr.next = bnext->hdr.next;
}

void split(block_t *b, uint16_t size)
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
 * ---------- public ----------
 */

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

    block_t *prev;
    block_t *b;

    b = (block_t *)list_find((list_item_t *)heap, (list_item_t **)&prev, match_free_block, size);

    if (b)
    {
        if (b->size - size > BLK_SIZE + MIN_CHUNK_SIZE)
            split(b, size);
        b->hdr.owner = owner;
        b->stat = ALLOCATED;
        return b->data;
    }
    else
        return NULL;
}

/*
 * free memory block
 */
void *mem_free(void *heap, void *p)
{

    block_t *prev;
    block_t *b;

    /* calculate block address from pointer */
    b = (block_t *)((uint16_t)p - BLK_SIZE);

    /* make sure it is a valid memory block by finding it */
    if (list_find((list_item_t *)heap, (list_item_t **)&prev, list_match_eq, (uint16_t)b))
    {
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
    else
        return NULL;
}