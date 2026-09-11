        ;; Default-heap malloc kept separate so hosted platform archives may
        ;; provide malloc without colliding with allocate().

        .module malloc
        .optsdcc -mz80 sdcccall(1)

        .globl  _malloc
        .globl  __libc_alloc_core
        .globl  __libc_active_heap
        .globl  __libc_default_heap
        .globl  __libc_heap_ready
        .globl  _heap_region
        .globl  _heap_init_arena

        .area   _CODE
_malloc::
        ld      a,h
        or      l
        jr      nz,.malloc_nonzero
        ld      de,#0
        ret
.malloc_nonzero:
        push    hl
        call    .heap_setup
        ld      hl,#__libc_default_heap
        ld      (__libc_active_heap),hl
        pop     hl
        jp      __libc_alloc_core

.heap_setup:
        ld      a,(__libc_heap_ready)
        or      a
        ret     nz
        call    _heap_region            ; HL = base, DE = limit
        inc     hl
        res     0,l
        ld      b,d
        ld      c,e
        ex      de,hl
        ld      hl,#__libc_default_heap
        push    bc
        call    _heap_init_arena
        ld      a,#1
        ld      (__libc_heap_ready),a
        ret
