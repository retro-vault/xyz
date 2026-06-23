        ;; conjf.s
        ;;
        ;; libc conjf() for the xcc Z80 libc.
        ;; float _Complex values are passed as two adjacent float words:
        ;;   real low/high at 4(ix)..7(ix), imag low/high at 8(ix)..11(ix).
        ;; The return ABI uses DE:HL for the real part and DE':HL' for the
        ;; imaginary part, so conjugation only needs to flip the sign bit of
        ;; the upper float before returning the untouched payload.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module conjf
        .optsdcc -mz80 sdcccall(1)

        .globl  _conjf

        .area   _CODE

_conjf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; Return the real component unchanged in DE:HL.
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)

        ;; Return the imaginary component with its sign bit toggled in DE':HL'.
        exx
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        ld      a,h
        xor     #0x80
        ld      h,a
        exx

        pop     ix
        ret
