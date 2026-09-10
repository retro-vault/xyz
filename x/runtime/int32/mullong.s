        ; Low 32 bits of a 32-by-32 multiply, signed or unsigned.
        ;
        ; Write a = a0 + 65536*a1 and b = b0 + 65536*b1. Modulo
        ; 2^32 the product is a0*b0 + 65536*(a1*b0 + a0*b1).
        ; The a1*b1 term and all sign corrections disappear. Keep the
        ; full low-word product and only the low words of the cross terms.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module mullong
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __mul32
        .globl  __mullong_rrx_s
        .globl  __mullong_rrf_s
        .globl  __mullong
        .globl  __mul16
        .globl  ___muluint2ulong

        ; inputs: de = a0, hl = a1; b at 4(ix)..7(ix) after saving ix
        ; outputs: de = product low word, hl = product high word
        ; clobbers: af, bc, de, hl; preserves ix, iy and alternate registers
        ; stack arguments remain for the caller to remove
__mul32:
__mullong_rrx_s::
__mullong_rrf_s::
__mullong:
        push    ix
        ld      ix, #0
        add     ix, sp
        push    iy
        push    de                      ; -4(ix): saved a0

        ld      e, 4(ix)
        ld      d, 5(ix)
        call    __mul16                 ; de = a1*b0, low word
        push    de                      ; -6(ix): first cross term

        ld      l, -4(ix)
        ld      h, -3(ix)
        ld      e, 6(ix)
        ld      d, 7(ix)
        call    __mul16                 ; de = a0*b1, low word
        pop     hl
        add     hl, de
        push    hl                      ; -6(ix): sum of cross terms

        ld      l, -4(ix)
        ld      h, -3(ix)
        ld      e, 4(ix)
        ld      d, 5(ix)
        call    ___muluint2ulong        ; hl:de = full a0*b0
        pop     bc
        add     hl, bc                  ; high half, modulo 65536

        pop     bc                      ; discard saved a0
        pop     iy
        pop     ix
        ret
