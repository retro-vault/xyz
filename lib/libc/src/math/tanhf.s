        ;; tanhf.s
        ;;
        ;; libc tanhf for the xcc Z80 libc.
        ;; Evaluates
        ;;   tanh(x) = sign(x) * (1 - 2 / (exp(2*|x|) + 1))
        ;; so large magnitudes naturally saturate toward +/-1 without creating
        ;; the inf/inf form that the direct sinh/cosh quotient would produce.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module tanhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _tanhf
        .globl  _expf
        .globl  ___fsadd
        .globl  ___fsdiv
        .globl  ___fsmul
        .globl  ___fssub

        .area   _CODE

_tanhf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-9
        add     hl,sp
        ld      sp,hl
        ;; Save sign(x), then continue with |x| so the exponential path only
        ;; has to reason about non-negative magnitudes.
        ld      a,b
        and     #0x80
        ld      -1(ix),a
        res     7,b
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),c
        ld      -6(ix),b

        ;; exp(2*|x|)
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-9(ix)
        ld      d,-8(ix)
        ld      l,-7(ix)
        ld      h,-6(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        call    _expf
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h

        ;; denom = exp(2*|x|) + 1
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-5(ix)
        ld      d,-4(ix)
        ld      l,-3(ix)
        ld      h,-2(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h

        ;; frac = 2 / denom
        ld      l,-3(ix)
        ld      h,-2(ix)
        push    hl
        ld      l,-5(ix)
        ld      h,-4(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x4000              ; 2.0f
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h

        ;; 1 - frac
        ld      l,-3(ix)
        ld      h,-2(ix)
        push    hl
        ld      l,-5(ix)
        ld      h,-4(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc

        ;; Restore the original sign. This also turns +0 into -0 for tanh(-0).
        ld      a,-1(ix)
        or      a
        jr      z,__tanhf_finish
        set     7,h
__tanhf_finish:
        ld      sp,ix
        pop     ix
        ret
