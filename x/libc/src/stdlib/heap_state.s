        ;; heap_state.s
        ;;
        ;; Shared state for the heap allocator.  Memory management is expressed
        ;; in terms of a heap descriptor (the "heap handle"):
        ;;
        ;;   struct heap {
        ;;       void  *head;    // +0  head of this heap's block list
        ;;       void  *base;    // +2  arena base
        ;;       void  *limit;   // +4  arena limit (one past the end)
        ;;       u8     bank;    // +6  reserved for banked memory
        ;;       u8     flags;   // +7  reserved
        ;;   };
        ;;
        ;; allocate()/deallocate() operate on a caller-supplied descriptor, so a
        ;; program may keep several heaps (banked blocks, a separate OS heap, a
        ;; per-process heap, ...).  malloc()/free() use __libc_default_heap.
        ;;
        ;; __libc_active_heap names the descriptor the inner first-fit / coalesce
        ;; logic currently works on; the entry points set it before running.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module heap_state
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_active_heap
        .globl  __libc_default_heap
        .globl  __libc_heap_ready
        .globl  __libc_heap_head_get

        .area   _DATA
__libc_active_heap::
        .dw     0
__libc_default_heap::
        .dw     0                       ; head
        .dw     0                       ; base
        .dw     0                       ; limit
        .db     0                       ; bank  (reserved)
        .db     0                       ; flags (reserved)
__libc_heap_ready::
        .db     0

        .area   _CODE
        ;; __libc_heap_head_get -> HL = active heap's free-list head
__libc_heap_head_get::
        ld      hl,(__libc_active_heap)
        ld      a,(hl)
        inc     hl
        ld      h,(hl)
        ld      l,a
        ret
