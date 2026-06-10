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

        .area   _DATA
__tanhf_x:     .ds 4
__tanhf_tmp:   .ds 4
__tanhf_sign:  .ds 1

        .area   _CODE

_tanhf::
        ;; Save sign(x), then continue with |x| so the exponential path only
        ;; has to reason about non-negative magnitudes.
        ld      a,h
        and     #0x80
        ld      (__tanhf_sign),a
        res     7,h
        ld      (__tanhf_x),de
        ld      (__tanhf_x + 2),hl

        ;; exp(2*|x|)
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__tanhf_x)
        ld      hl,(__tanhf_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        call    _expf
        ld      (__tanhf_tmp),de
        ld      (__tanhf_tmp + 2),hl

        ;; denom = exp(2*|x|) + 1
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__tanhf_tmp)
        ld      hl,(__tanhf_tmp + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__tanhf_tmp),de
        ld      (__tanhf_tmp + 2),hl

        ;; frac = 2 / denom
        ld      hl,(__tanhf_tmp + 2)
        push    hl
        ld      hl,(__tanhf_tmp)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x4000              ; 2.0f
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__tanhf_tmp),de
        ld      (__tanhf_tmp + 2),hl

        ;; 1 - frac
        ld      hl,(__tanhf_tmp + 2)
        push    hl
        ld      hl,(__tanhf_tmp)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc

        ;; Restore the original sign. This also turns +0 into -0 for tanh(-0).
        ld      a,(__tanhf_sign)
        or      a
        ret     z
        set     7,h
        ret

