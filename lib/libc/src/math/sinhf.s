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

        .area   _CODE

_sinhf::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; save x high word at -2(ix),-1(ix)
        push    de                      ; save x low word at -4(ix),-3(ix)

        ;; exp(+x)
        call    _expf
        push    hl                      ; save exp(+x) high word at -6(ix),-5(ix)
        push    de                      ; save exp(+x) low word at -8(ix),-7(ix)

        ;; exp(-x)
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,h
        xor     #0x80
        ld      h,a
        call    _expf

        ;; exp(x) - exp(-x)
        push    hl
        push    de                      ; stack operand = exp(-x)
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)                ; a = exp(+x)
        call    ___fssub
        pop     bc
        pop     bc

        ;; Scale the antisymmetric difference by 1/2.
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret
