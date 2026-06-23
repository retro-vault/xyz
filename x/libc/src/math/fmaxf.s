        ;; fmaxf.s
        ;;
        ;; libc fmaxf implementation for the xcc Z80 libc.
        ;; Returns the larger of two single-precision values.  NaN inputs are
        ;; not handled specially (this runtime has no NaN support).  Shares the
        ;; sign/magnitude comparison core with fminf.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih




        .module fmaxf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmaxf
        .globl  __float_cmp_xy
        .globl  __float_load_y

        .area   _CODE
_fmaxf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __float_cmp_xy          ; A = +1 (x>y), 0 (==), 0xFF (x<y)
        inc     a                       ; 0xFF -> 0 when x < y
        jr      nz,fmaxf_keep_x
        call    __float_load_y          ; x < y: return y
fmaxf_keep_x:
        pop     ix
        ret

        ;; __float_cmp_xy
        ;; inputs:  HL:DE = x, y at 4(ix)..7(ix), IX = frame
        ;; outputs: A = sign of (x - y): 0x01 if x>y, 0x00 if equal, 0xFF if x<y
        ;; clobbers: AF, BC
        ;; notes: sign/magnitude ordering; +/-0 compare equal.
