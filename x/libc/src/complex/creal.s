        ;; creal.s
        ;;
        ;; libc crealf()/creal()/creall() backend for the xcc Z80 libc.
        ;; float _Complex arguments are laid out as two adjacent float words:
        ;;   real low/high at 4(ix)..7(ix), imag low/high at 8(ix)..11(ix).
        ;; Scalar float returns use DE = low 16 bits and HL = high 16 bits.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module creal
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __creal

__creal:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      e, 4(ix)
        ld      d, 5(ix)
        ld      l, 6(ix)
        ld      h, 7(ix)
        pop     ix
        ret
