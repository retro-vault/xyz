        ;; c8rtomb.s
        ;; Split from mbrtoc16.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module c8rtomb
        .optsdcc -mz80 sdcccall(1)

        .globl  _c8rtomb

        .area   _CODE
_c8rtomb::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; [s]
        push    de                      ; [c8]
        ld      l,4(ix)
        ld      h,5(ix)
        ld      a,h
        or      l
        jr      z,c8_nr
        ld      (hl),#0
        inc     hl
        ld      (hl),#0
c8_nr:
        pop     de                      ; [c8] — E = value, D should be 0 but ignore for char8
        pop     hl                      ; [s]
        ld      a,h
        or      l
        jr      z,c8_ret1
        ld      (hl),e
c8_ret1:
        ld      de,#1
        pop     ix
        ret
