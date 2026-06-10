        ;; coshf.s
        ;;
        ;; libc coshf for the xcc Z80 libc.
        ;; Uses the defining identity
        ;;   cosh(x) = (exp(x) + exp(-x)) / 2
        ;; on top of the existing expf and float runtime helpers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module coshf
        .optsdcc -mz80 sdcccall(1)

        .globl  _coshf
        .globl  _expf
        .globl  ___fsadd
        .globl  ___fsmul

        .area   _DATA
__coshf_x:   .ds 4
__coshf_ep:  .ds 4
__coshf_en:  .ds 4

        .area   _CODE

_coshf::
        ld      (__coshf_x),de
        ld      (__coshf_x + 2),hl

        ;; exp(+x)
        call    _expf
        ld      (__coshf_ep),de
        ld      (__coshf_ep + 2),hl

        ;; exp(-x)
        ld      de,(__coshf_x)
        ld      hl,(__coshf_x + 2)
        ld      a,h
        xor     #0x80
        ld      h,a
        call    _expf
        ld      (__coshf_en),de
        ld      (__coshf_en + 2),hl

        ;; exp(x) + exp(-x)
        ld      hl,(__coshf_en + 2)
        push    hl
        ld      hl,(__coshf_en)
        push    hl
        ld      de,(__coshf_ep)
        ld      hl,(__coshf_ep + 2)
        call    ___fsadd
        pop     bc
        pop     bc

        ;; Scale the symmetric sum by 1/2.
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ret

