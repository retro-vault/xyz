        ;; cimag.s
        ;;
        ;; libc cimagf()/cimag()/cimagl() backend for the xcc Z80 libc.
        ;; float _Complex arguments are laid out as two adjacent float words:
        ;;   real low/high at 4(ix)..7(ix), imag low/high at 8(ix)..11(ix).
        ;; Scalar float returns use DE = low 16 bits and HL = high 16 bits.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cimag
        .area   _CODE
        .globl  __cimag

__cimag:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      e, 8(ix)
        ld      d, 9(ix)
        ld      l, 10(ix)
        ld      h, 11(ix)
        pop     ix
        ret
