        ; Internal named-service lookup.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _svc_query
        .optsdcc -mz80 sdcccall(1)
        .globl  __svc_query
        .globl  __svc_first
        .globl  __string_compare
        .area   _CODE

        ; hl = name; returns de = function table or zero.
__svc_query::
        push    ix
        push    iy
        push    hl
        pop     ix
        ld      iy, (__svc_first)
        ld      b, #0
.loop:
        push    iy
        pop     de
        ld      a, d
        or      e
        jr      z, .done
        inc     de
        inc     de
        inc     de
        inc     de
        push    ix
        pop     hl
        push    bc
        call    __string_compare
        pop     bc
        ld      a, d
        or      e
        jr      z, .found
        ld      l, 0(iy)
        ld      h, 1(iy)
        push    hl
        pop     iy
        djnz    .loop
        ld      de, #0
        jr      .done
.found:
        ld      e, 20(iy)
        ld      d, 21(iy)
.done:
        pop     iy
        pop     ix
        ret
