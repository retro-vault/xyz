        ;; copysignf.s
        ;;
        ;; libc copysignf for the xcc Z80 libc.
        ;; Returns the magnitude of x with the sign bit copied from y.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih
        .module copysignf
        .optsdcc -mz80 sdcccall(1)
        .globl  _copysignf
        .area   _CODE
        ;; IEEE-754 single stores the sign in bit 7 of the top byte.
        ;; x arrives in HL:DE; y is stacked at 4(ix)..7(ix).
_copysignf::
        push    ix
        ld      ix,#0
        add     ix,sp
        res     7,h                     ; start from +|x|
        bit     7,7(ix)                 ; copy y's sign from byte a3
        jr      z,cps_done              ; positive y leaves the sign clear
        set     7,h                     ; negative y makes the result negative
cps_done:
        pop     ix
        ret
