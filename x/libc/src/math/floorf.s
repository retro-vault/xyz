        ;; floorf.s
        ;;
        ;; libc floorf implementation for the xcc Z80 libc.
        ;; Rounds toward negative infinity.  double / long double are 32-bit
        ;; on this target, so floor / floorl share the implementation.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module floorf
        .optsdcc -mz80 sdcccall(1)


        .globl  _floorf
        .globl  _truncf
        .globl  __float_incmag

        .area   _CODE

        ;; _floorf / _floor / _floorl
        ;; inputs:  HL:DE = float x
        ;; outputs: HL:DE = floor(x)
        ;; clobbers: AF, BC, IX
_floorf::
        push    de                      ; save x low
        push    hl                      ; save x high
        call    _truncf                 ; HL:DE = trunc(x)
        pop     bc                      ; BC = x high (B=H, C=L)
        ld      a,h
        cp      b
        jr      nz,floorf_frac_hl
        ld      a,l
        cp      c
        jr      nz,floorf_frac_hl
        pop     bc                      ; BC = x low (B=D, C=E)
        ld      a,d
        cp      b
        jr      nz,floorf_frac
        ld      a,e
        cp      c
        jr      nz,floorf_frac
        ret                             ; no fraction: floor == trunc
floorf_frac_hl:
        pop     bc                      ; discard saved x low
floorf_frac:
        bit     7,h                     ; sign of trunc(x)
        ret     z                       ; x >= 0: floor == trunc
        jp      __float_incmag          ; x < 0 with fraction: more negative
