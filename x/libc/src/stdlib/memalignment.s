        ;; memalignment.s
        ;; C23 memalignment(ptr): report the largest power-of-two alignment
        ;; visible from the pointer value.

        .module memalignment
        .optsdcc -mz80 sdcccall(1)

        .globl  _memalignment

        .area   _CODE
_memalignment::
        ld      a,h
        or      l
        jr      z,memalignment_zero
        ld      b,h
        ld      c,l                     ; BC = original pointer value
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl                      ; HL = -ptr
        ld      a,l
        and     c
        ld      e,a
        ld      a,h
        and     b
        ld      d,a                     ; DE = ptr & -ptr
        ret

memalignment_zero:
        ld      de,#0
        ret
