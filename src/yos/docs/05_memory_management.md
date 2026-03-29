# Memory Management

*Yos* provides a simple heap allocator that supports multiple independent heaps, block splitting, and block coalescing. The implementation lives in `kernel/mem.c` and `kernel/mem.h`.

## Two Heaps

*Yos* maintains two separate heap areas:

| Symbol | Size | Purpose |
|---|---|---|
| `__sys_heap` | 1024 bytes | OS-internal allocations (threads, timers, events, ...) |
| `__heap` | rest of RAM | User processes, thread stacks, application data |

Both heaps are initialised in `main()`:

```c
mem_init((void *)&_sys_heap, 1024);
mem_init((void *)&_heap, 0xffff - (uint16_t)&_heap);
```

The second call hands everything from the end of the system heap to the top of the 64 KB address space to the user heap. On a standard 48K ZX Spectrum this is roughly 42 KB of usable application memory.

Having two separate heaps means OS allocations and user allocations never interfere with each other. A runaway user program that exhausts the user heap will not crash the kernel.

## The Block Header (`block_t`)

Every allocation is preceded by a `block_t` header that the allocator uses to track the block's size, status, and owner:

```c
typedef struct block_s {
    sysobj_t hdr;       /* list link + owner (4 bytes) */
    uint8_t  stat;      /* ALLOCATED or NEW (free) */
    uint16_t size;      /* usable payload size in bytes */
    uint8_t  data[1];   /* payload begins here */
} block_t;
```

`BLK_SIZE` is the size of a `block_t` *excluding* the payload byte:

```c
#define BLK_SIZE (sizeof(struct block_s) - sizeof(uint8_t[1]))
```

In memory, an allocated 20-byte block looks like:

```
block_t header (BLK_SIZE bytes)          payload (20 bytes)
┌────────┬───────┬──────┬──────┬─────────────────────────┐
│  next  │ owner │ stat │ size │  your data here          │
│ 2 bytes│2 bytes│1 byte│2 byte│  20 bytes                │
└────────┴───────┴──────┴──────┴─────────────────────────┘
          ↑                     ↑
       block_t *              returned pointer (data field)
```

`mem_allocate` returns a pointer to `data`, not to the start of the `block_t`. `mem_free` recovers the `block_t` address by subtracting `BLK_SIZE` from the pointer it receives.

## Initialising a Heap

`mem_init` turns a raw memory region into a single large free block that the allocator can then subdivide:

```c
void mem_init(void *heap, uint16_t size);
```

```c
/* Example: initialise a 2048-byte heap at address 0xC000 */
mem_init((void *)0xC000, 2048);
```

After `mem_init`, the region contains one `block_t` marked as free (`NEW`), with `size` set to `total_size - BLK_SIZE` (the header itself takes up some space), and `next` set to `NULL`.

## Allocating Memory

```c
void *mem_allocate(void *heap, uint16_t size, void *owner);
```

`mem_allocate` uses a **first-fit** strategy: it walks the linked list of blocks starting from `heap` and returns the first free block large enough to satisfy the request.

If the found block is significantly larger than needed (more than `BLK_SIZE + MIN_CHUNK_SIZE` = 4 bytes larger), the allocator **splits** it:

```
Before split:
┌─────────────────────────────────────────────────┐
│ free block, size = 100                          │
└─────────────────────────────────────────────────┘

After mem_allocate(heap, 20, owner):
┌──────────────────────┬──────────────────────────┐
│ allocated, size = 20 │ free, size = 100-20-BLK  │
└──────────────────────┴──────────────────────────┘
         ↑                        ↑
    returned to caller       remainder stays free
```

The minimum chunk size (`MIN_CHUNK_SIZE = 4`) prevents creating free blocks so small they would be useless and just waste header space.

On success, `mem_allocate` returns a pointer to the payload. On failure (no block large enough), it returns `NULL`. **Always check the return value.**

```c
/* Allocate a 512-byte thread stack from the user heap */
void *stack = mem_allocate((void *)&_heap, 512, (void *)owner_thread);
if (!stack) {
    /* handle allocation failure */
}
```

## Freeing Memory

```c
void *mem_free(void *heap, void *p);
```

`mem_free` takes the pointer returned by `mem_allocate` (the `data` field), recovers the `block_t` header by subtracting `BLK_SIZE`, marks the block as free, and attempts to **coalesce** adjacent free blocks to prevent fragmentation.

The coalescing logic merges up to three blocks at a time:

```
Before free of middle block:
┌──────────────┬──────────────┬──────────────┐
│ free block A │ allocated B  │ free block C │
└──────────────┴──────────────┴──────────────┘

After mem_free(heap, B->data):
┌──────────────────────────────────────────────┐
│            single merged free block          │
└──────────────────────────────────────────────┘
```

Steps performed:
1. Mark B as free (`stat = NEW`, `owner = NONE`).
2. If the *previous* block (A) is also free, merge A and B into one block.
3. If the *next* block (C) is also free, merge the result with C.

This **boundary-tag** style coalescing keeps the heap from degenerating into many tiny unusable fragments over time.

`mem_free` returns the payload pointer of the freed block on success, or `NULL` if `p` was not found in the heap (a safety check against double-free or invalid pointers).

## Heap Fragmentation

The ZX Spectrum has 64 KB of address space. Memory is precious. Keep these guidelines in mind:

- **Allocate once, keep long-lived objects alive.** Repeatedly allocating and freeing small blocks of varying sizes leads to fragmentation even with coalescing.
- **Use the OS heap for OS objects only.** Thread stacks, timer hooks, events — pass `(void *)&_sys_heap` as the heap pointer. For application data, use `(void *)&_heap`.
- **Thread stacks are freed when a thread exits** (as part of resource accounting). Do not free a stack manually.
- **The minimum useful allocation is `MIN_CHUNK_SIZE = 4` bytes** of payload. Smaller requests will still be granted but the block cannot be split further.

## Usage Example: Custom Heap

You can create additional heaps anywhere in RAM. This is useful for isolating a subsystem's allocations:

```c
/* A dedicated 256-byte heap for graphics scratch buffers */
uint8_t gfx_heap_area[256];
mem_init(gfx_heap_area, sizeof(gfx_heap_area));

/* Allocate from it */
uint8_t *buf = mem_allocate(gfx_heap_area, 64, NONE);

/* Free from it */
mem_free(gfx_heap_area, buf);
```

The heap pointer (`gfx_heap_area` in this example) must always be passed to every `mem_allocate` and `mem_free` call that belongs to that heap. Mixing up heap pointers will corrupt both heaps.
