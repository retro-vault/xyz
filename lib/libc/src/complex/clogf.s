        ;; clogf.s
        ;;
        ;; libc clogf() for the xcc Z80 libc.
        ;; Uses the principal-value identity
        ;;   log(x + i y) = log(|z|) + i * atan2(y, x)
        ;; via the existing cabsf(), cargf(), and logf() helpers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module clogf
        .optsdcc -mz80 sdcccall(1)

        .globl  _clogf
        .globl  _cabsf
        .globl  _cargf
        .globl  _logf

        .area   _DATA
__clogf_real:
        .ds     4
__clogf_imag:
        .ds     4

        .area   _CODE

_clogf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; real = logf(cabsf(z))
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        call    _cabsf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        call    _logf
        ld      (__clogf_real),de
        ld      (__clogf_real + 2),hl

        ;; imag = cargf(z)
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        call    _cargf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      (__clogf_imag),de
        ld      (__clogf_imag + 2),hl

        ;; Return the packed complex result.
        ld      de,(__clogf_real)
        ld      hl,(__clogf_real + 2)
        exx
        ld      de,(__clogf_imag)
        ld      hl,(__clogf_imag + 2)
        exx

        pop     ix
        ret
