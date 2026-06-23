        ;; ilogbf.s
        ;;
        ;; libc ilogbf implementation for the xcc Z80 libc.
        ;; Returns the unbiased exponent of x as an int.  Zero returns the
        ;; target's FP_ILOGB0 sentinel (INT_MIN for this 16-bit int).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ilogbf
        .optsdcc -mz80 sdcccall(1)


        .globl  _ilogbf
        .area   _CODE

        ;; _ilogbf / _ilogb / _ilogbl  (32-bit float types on this target)
        ;; inputs:  HL:DE = float x
        ;; outputs: DE = floor(log2(|x|)) = exp8 - 127, or INT_MIN if x == 0
        ;; clobbers: AF
_ilogbf::
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,ilogbf_exp_ok
        inc     a
ilogbf_exp_ok:
        or      a
        jr      z,ilogbf_zero
        sub     #127                    ; A = unbiased exponent (signed)
        ld      e,a
        rla                             ; sign bit -> carry
        sbc     a,a                     ; A = 0xFF if negative, else 0x00
        ld      d,a                     ; sign-extend into DE
        ret
ilogbf_zero:
        ld      de,#0x8000              ; FP_ILOGB0 = INT_MIN (16-bit)
        ret
