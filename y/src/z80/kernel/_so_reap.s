        ; Destroy every system object belonging to an owner.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _so_reap
        .optsdcc -mz80 sdcccall(1)
        .globl  __so_reap
        .globl  __process_find_owned
        .globl  _so_destroy
        .area   _CODE

        ; inputs: hl = list head address, de = owner
        ; outputs: none; caller holds a critical section
        ; clobbers: af, bc, de, hl; preserves ix and iy
__so_reap::
        push    iy
        push    hl
        pop     iy
.loop:
        push    de                      ; retain owner across lookup/destruction
        ld      l, 0(iy)
        ld      h, 1(iy)
        call    __process_find_owned
        ld      a, d
        or      e
        jr      z, .done
        push    iy
        pop     hl
        call    _so_destroy
        pop     de
        jr      .loop
.done:
        pop     de
        pop     iy
        ret
