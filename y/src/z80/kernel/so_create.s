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

        ; hl = head address, de = size, owner at sp+2; removes owner.
_so_create::
        push    ix
        ld      ix, #0
        add     ix, sp
        push    hl
        ld      c, 4(ix)
        ld      b, 5(ix)
        push    bc
        ld      hl, #__sys_heap
        call    _mem_allocate
        pop     hl
        ld      a, d
        or      e
        jr      z, .done
        push    de
        call    _list_insert
        pop     de
        ld      hl, #2
        add     hl, de
        ld      a, 4(ix)
        ld      (hl), a
        inc     hl
        ld      a, 5(ix)
        ld      (hl), a
.done:
        pop     ix
        pop     hl
        pop     bc
        jp      (hl)
