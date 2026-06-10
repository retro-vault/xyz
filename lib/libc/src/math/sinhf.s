        ;; sinhf.s
        ;;
        ;; libc sinhf for the xcc Z80 libc.
        ;; Uses the defining identity
        ;;   sinh(x) = (exp(x) - exp(-x)) / 2
        ;; on top of the existing expf and float runtime helpers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sinhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _sinhf
        .globl  _expf
        .globl  ___fsmul
        .globl  ___fssub

        .area   _DATA
__sinhf_x:   .ds 4
__sinhf_ep:  .ds 4
__sinhf_en:  .ds 4

        .area   _CODE

_sinhf::
        ld      (__sinhf_x),de
        ld      (__sinhf_x + 2),hl

        ;; exp(+x)
        call    _expf
        ld      (__sinhf_ep),de
        ld      (__sinhf_ep + 2),hl

        ;; exp(-x)
        ld      de,(__sinhf_x)
        ld      hl,(__sinhf_x + 2)
        ld      a,h
        xor     #0x80
        ld      h,a
        call    _expf
        ld      (__sinhf_en),de
        ld      (__sinhf_en + 2),hl

        ;; exp(x) - exp(-x)
        ld      hl,(__sinhf_en + 2)
        push    hl
        ld      hl,(__sinhf_en)
        push    hl
        ld      de,(__sinhf_ep)
        ld      hl,(__sinhf_ep + 2)
        call    ___fssub
        pop     bc
        pop     bc

        ;; Scale the antisymmetric difference by 1/2.
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ret
