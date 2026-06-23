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

        .area   _CODE

_atanhf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-13
        add     hl,sp
        ld      sp,hl
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),c
        ld      -10(ix),b              ; x
        ld      e,-13(ix)
        ld      d,-12(ix)
        ld      l,-11(ix)
        ld      h,-10(ix)
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> preserve payload/sign
        jp      z,atanhf_ret_x
        cp      #2                      ; +/-0 -> preserve signed zero
        jp      z,atanhf_ret_x
        cp      #1                      ; +/-Inf -> outside domain
        jp      z,atanhf_ret_nan

        ld      a,-10(ix)
        and     #0x80
        ld      -1(ix),a                ; sign
        ld      a,-10(ix)
        and     #0x7f
        ld      -6(ix),a
        ld      a,-13(ix)
        ld      -9(ix),a
        ld      a,-12(ix)
        ld      -8(ix),a
        ld      a,-11(ix)
        ld      -7(ix),a                ; abs

        ;; Compare |x| with 1 to split the finite domain.
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-9(ix)
        ld      d,-8(ix)
        ld      l,-7(ix)
        ld      h,-6(ix)
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jp      z,atanhf_domain_ok      ; |x| < 1
        ld      a,d
        or      e
        jp      z,atanhf_ret_inf        ; |x| == 1
        jp      atanhf_ret_nan          ; |x| > 1

atanhf_domain_ok:
        ;; tmp = 1 + |x|
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-9(ix)
        ld      d,-8(ix)
        ld      l,-7(ix)
        ld      h,-6(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h                ; tmp

        ;; (1 + |x|) / (1 - |x|)
        ld      l,-7(ix)
        ld      h,-6(ix)
        push    hl
        ld      l,-9(ix)
        ld      h,-8(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      e,-5(ix)
        ld      d,-4(ix)
        ld      l,-3(ix)
        ld      h,-2(ix)
        call    ___fsdiv
        pop     bc
        pop     bc

        ;; 0.5 * log(...)
        call    _logf
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-5(ix)
        ld      d,-4(ix)
        ld      l,-3(ix)
        ld      h,-2(ix)
        call    ___fsmul
        pop     bc
        pop     bc

        ld      a,-1(ix)
        or      a
        jr      z,atanhf_ret_value
        set     7,h

atanhf_ret_value:
        ld      sp,ix
        pop     ix
        ret

atanhf_ret_inf:
        ld      hl,#0x7f80
        ld      de,#0x0000
        ld      a,-1(ix)
        or      a
        jr      z,atanhf_ret_inf_done
        set     7,h

atanhf_ret_inf_done:
        ld      sp,ix
        pop     ix
        ret

atanhf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret

atanhf_ret_x:
        ld      e,-13(ix)
        ld      d,-12(ix)
        ld      l,-11(ix)
        ld      h,-10(ix)
        ld      sp,ix
        pop     ix
        ret
