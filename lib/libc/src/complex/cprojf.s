        ;; cprojf.s
        ;;
        ;; libc cprojf() for the xcc Z80 libc.
        ;;
        ;; Finite inputs project to themselves. Any complex infinity projects to
        ;;   +INF + I*copysign(0, imag(z))
        ;; which is the C-standard branch-cut preserving form.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cprojf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cprojf
        .globl  ___libc_isinff

        .area   _CODE

_cprojf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; If either component is infinite, project onto the real infinity.
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___libc_isinff
        ld      a,d
        or      e
        jr      nz,cprojf_inf

        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    ___libc_isinff
        ld      a,d
        or      e
        jr      nz,cprojf_inf

        ;; Finite inputs project to themselves unchanged.
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)

        exx
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        exx

        pop     ix
        ret

cprojf_inf:
        ;; Return +INF as the real component.
        ld      de,#0x0000
        ld      hl,#0x7f80

        ;; Preserve the sign of the imaginary zero in the alternate return.
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        bit     7,11(ix)
        jr      z,cprojf_inf_done
        set     7,h
cprojf_inf_done:
        exx

        pop     ix
        ret
