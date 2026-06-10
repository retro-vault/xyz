        ;; atanhf.s
        ;;
        ;; libc atanhf for the xcc Z80 libc.
        ;;
        ;; Use the odd magnitude form
        ;;   atanh(x) = sign(x) * 0.5 * log((1 + |x|) / (1 - |x|))
        ;; for |x| < 1. Inputs with |x| == 1 return signed infinities; larger
        ;; magnitudes return a quiet NaN.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module atanhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _atanhf
        .globl  ___libc_fpclassifyf
        .globl  ___fscmp
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsdiv
        .globl  ___fsmul
        .globl  _logf

        .area   _DATA
__atanhf_x:      .ds 4
__atanhf_abs:    .ds 4
__atanhf_tmp:    .ds 4
__atanhf_sign:   .ds 1

        .area   _CODE

_atanhf::
        ld      (__atanhf_x),de
        ld      (__atanhf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> preserve payload/sign
        jp      z,atanhf_ret_x
        cp      #2                      ; +/-0 -> preserve signed zero
        jp      z,atanhf_ret_x
        cp      #1                      ; +/-Inf -> outside domain
        jp      z,atanhf_ret_nan

        ld      a,h
        and     #0x80
        ld      (__atanhf_sign),a
        res     7,h
        ld      (__atanhf_abs),de
        ld      (__atanhf_abs + 2),hl

        ;; Compare |x| with 1 to split the finite domain.
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__atanhf_abs)
        ld      hl,(__atanhf_abs + 2)
        call    ___fscmp
        pop     bc
        pop     bc
        ld      a,d
        cp      #0xff
        jr      z,atanhf_domain_ok      ; |x| < 1
        ld      a,d
        or      e
        jr      z,atanhf_ret_inf        ; |x| == 1
        jr      atanhf_ret_nan          ; |x| > 1

atanhf_domain_ok:
        ;; tmp = 1 + |x|
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__atanhf_abs)
        ld      hl,(__atanhf_abs + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__atanhf_tmp),de
        ld      (__atanhf_tmp + 2),hl

        ;; (1 + |x|) / (1 - |x|)
        ld      hl,(__atanhf_abs + 2)
        push    hl
        ld      hl,(__atanhf_abs)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      de,(__atanhf_tmp)
        ld      hl,(__atanhf_tmp + 2)
        call    ___fsdiv
        pop     bc
        pop     bc

        ;; 0.5 * log(...)
        call    _logf
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc

        ld      a,(__atanhf_sign)
        or      a
        ret     z
        set     7,h
        ret

atanhf_ret_inf:
        ld      hl,#0x7f80
        ld      de,#0x0000
        ld      a,(__atanhf_sign)
        or      a
        ret     z
        set     7,h
        ret

atanhf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ret

atanhf_ret_x:
        ld      de,(__atanhf_x)
        ld      hl,(__atanhf_x + 2)
        ret
