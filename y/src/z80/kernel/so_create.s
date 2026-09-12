        ; Allocate and link a system object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module so_create
        .optsdcc -mz80 sdcccall(1)
        .globl  _so_create
        .globl  __sys_heap
        .globl  _mem_allocate
        .globl  _list_insert
        .area   _CODE

        ; inputs: hl = head, de = size, owner at sp+2; removes owner
        ; outputs: de = object or zero
        ; clobbers: af, bc, hl; preserves ix and iy
        ; Caller must hold a critical section through payload initialization.
_so_create::
        push    hl
        ld      hl, #4
        add     hl, sp
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        push    bc
        ld      hl, #__sys_heap
        call    _mem_allocate
        pop     hl
        ld      a, d
        or      e
        jr      z, .done
        call    _list_insert
        ld      hl, #-5                ; reuse heap header's owner
        add     hl, de
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        ld      hl, #2
        add     hl, de
        ld      (hl), c
        inc     hl
        ld      (hl), b
.done:
        pop     hl
        pop     bc
        jp      (hl)
