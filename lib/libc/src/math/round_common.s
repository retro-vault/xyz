        ;; round_common.s
        ;;
        ;; Shared helpers for the single-precision rounding routines
        ;; (floorf/ceilf/roundf), operating on the float layout HL:DE
        ;; (H=a3 sign/exp, L=a2, D=a1, E=a0).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module round_common
        .optsdcc -mz80 sdcccall(1)


        .globl  __float_exp8
        .globl  __float_incmag

        .area   _CODE

        ;; __float_exp8
        ;; inputs:  HL:DE = float
        ;; outputs: A = biased 8-bit exponent
        ;; clobbers: AF
__float_exp8::
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        ret     z
        inc     a
        ret

        ;; __float_incmag
        ;; Adds 1 to the magnitude of an integer-valued float.  trunc(+/-0)
        ;; becomes +/-1.0.  Used to round truncated values away from zero.
        ;; inputs:  HL:DE = integer-valued float (exponent < 150)
        ;; outputs: HL:DE = float with |value| increased by one
        ;; clobbers: AF, BC, IX
        ;; frame: -4 sig low, -3 sig mid, -2 sig high (significand bytes)
__float_incmag::
        call    __float_exp8
        or      a
        jr      nz,incmag_normal
        ;; +/-0 -> +/-1.0
        ld      a,h
        and     #0x80
        or      #0x3f
        ld      h,a
        ld      l,#0x80
        ld      d,#0
        ld      e,#0
        ret
incmag_normal:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-4
        add     hl,sp
        ld      sp,hl
        ld      h,b
        ld      l,c
        ;; significand bytes: low=E, mid=D, high=0x80|(L&0x7F)
        ld      -4(ix),e
        ld      -3(ix),d
        ld      a,l
        and     #0x7f
        or      #0x80
        ld      -2(ix),a
        ;; k = 150 - exp8  (1..23) = bit index of integer LSB
        call    __float_exp8
        ld      b,a
        ld      a,#150
        sub     b
        ld      b,a                     ; B = k
        ;; addbyte = 1 << (k & 7)
        and     #7
        ld      c,a
        ld      a,#1
        inc     c
incmag_bit:
        dec     c
        jr      z,incmag_bit_done
        add     a,a
        jr      incmag_bit
incmag_bit_done:
        ld      c,a                     ; C = addbyte
        ;; byte index = k >> 3  (0,1,2)
        ld      a,b
        srl     a
        srl     a
        srl     a
        or      a
        jr      z,incmag_add_lo
        dec     a
        jr      z,incmag_add_mid
        ;; index 2: high byte
        ld      a,-2(ix)
        add     a,c
        ld      -2(ix),a
        jr      incmag_after
incmag_add_mid:
        ld      a,-3(ix)
        add     a,c
        ld      -3(ix),a
        ld      a,-2(ix)
        adc     a,#0
        ld      -2(ix),a
        jr      incmag_after
incmag_add_lo:
        ld      a,-4(ix)
        add     a,c
        ld      -4(ix),a
        ld      a,-3(ix)
        adc     a,#0
        ld      -3(ix),a
        ld      a,-2(ix)
        adc     a,#0
        ld      -2(ix),a
incmag_after:
        jr      c,incmag_overflow
        ;; repack significand: E=low, D=mid, L=(L&0x80)|(high&0x7F)
        ld      a,-4(ix)
        ld      e,a
        ld      a,-3(ix)
        ld      d,a
        ld      a,-2(ix)
        and     #0x7f
        ld      c,a
        ld      a,l
        and     #0x80
        or      c
        ld      l,a
        ld      sp,ix
        pop     ix
        ret
incmag_overflow:
        ;; carried out of bit 23: result is the next power of two; mantissa 0,
        ;; exponent incremented.
        ld      d,#0
        ld      e,#0
        call    __float_exp8            ; original exp8 (H:L unchanged)
        inc     a                       ; exp8 + 1
        ld      b,a
        srl     a
        ld      c,a                     ; (exp8+1) >> 1
        ld      a,h
        and     #0x80
        or      c
        ld      h,a
        ld      a,b
        and     #1
        rrca                            ; low bit -> bit 7
        ld      l,a
        ld      sp,ix
        pop     ix
        ret
