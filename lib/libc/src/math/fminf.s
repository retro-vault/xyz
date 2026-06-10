        ;; fminf.s
        ;;
        ;; libc fminf implementation for the xcc Z80 libc.
        ;; Returns the smaller of two single-precision values, reusing the
        ;; comparison core from fmaxf.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fminf
        .optsdcc -mz80 sdcccall(1)


        .globl  _fminf
        .globl  __float_cmp_xy
        .globl  __float_load_y

        .area   _CODE

        ;; _fminf / _fmin / _fminl  (32-bit float types on this target)
        ;; inputs:  HL:DE = x, y on stack (4(ix)..7(ix))
        ;; outputs: HL:DE = min(x, y)
        ;; clobbers: AF, BC, IX
_fminf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __float_cmp_xy          ; A = +1 (x>y), 0 (==), 0xFF (x<y)
        dec     a                       ; 0x01 -> 0 when x > y
        jr      nz,fminf_keep_x
        call    __float_load_y          ; x > y: return y
fminf_keep_x:
        pop     ix
        ret
