        ; Obtain the caller's process without changing its arguments.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _current_process
        .optsdcc -mz80 sdcccall(1)
        .globl  __current_process
        .globl  _thread_current
        .area   _CODE

        ; outputs: bc = effective resource owner, zero in kernel context
        ; clobbers: af, bc; preserves de, hl, ix and iy
__current_process::
        push    hl
        ld      hl, (_thread_current)
        ld      b, h
        ld      c, l
        ld      a, h
        or      l
        jr      z, .done
        inc     hl
        inc     hl
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        ld      a, b
        or      c
        jr      nz, .done
        ld      bc, #19
        add     hl, bc
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
.done:
        pop     hl
        ret
