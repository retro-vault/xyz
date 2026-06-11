        ;; cbrtf.s
        ;;
        ;; Computes the real cube root through:
        ;;   cbrt(x) = sign(x) * exp(log(|x|) / 3)
        ;; Special values and signed zero are preserved explicitly before the
        ;; logarithm path runs.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cbrtf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cbrtf
        .globl  ___libc_fpclassifyf
        .globl  ___libc_signbitf
        .globl  ___fsmul
        .globl  __libc_logf_core
        .globl  __libc_expf_core

        .area   _CODE

CBR_XLO  .equ -5
CBR_XHI  .equ -3
CBR_SIGN .equ -1

_cbrtf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-5
        add     hl,sp
        ld      sp,hl
        ld      CBR_XLO(ix),e
        ld      CBR_XLO+1(ix),d
        ld      CBR_XHI(ix),l
        ld      CBR_XHI+1(ix),h
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> return unchanged
        jr      z,cbrtf_ret_x
        cp      #1                      ; Inf -> return unchanged
        jr      z,cbrtf_ret_x
        cp      #2                      ; Zero -> preserve signed zero
        jr      z,cbrtf_ret_x

        ld      e,CBR_XLO(ix)
        ld      d,CBR_XLO+1(ix)
        ld      l,CBR_XHI(ix)
        ld      h,CBR_XHI+1(ix)
        call    ___libc_signbitf
        ld      a,d
        or      e
        ld      a,#0
        jr      z,cbrtf_have_sign
        ld      a,#1
cbrtf_have_sign:
        ld      CBR_SIGN(ix),a

        ld      e,CBR_XLO(ix)
        ld      d,CBR_XLO+1(ix)
        ld      l,CBR_XHI(ix)
        ld      h,CBR_XHI+1(ix)
        ld      a,CBR_SIGN(ix)
        or      a
        jr      z,cbrtf_positive
        ld      a,h
        xor     #0x80
        ld      h,a
cbrtf_positive:
        call    __libc_logf_core
        ld      hl,#0x3eaa              ; 1/3
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        call    __libc_expf_core
        ld      a,CBR_SIGN(ix)
        or      a
        jr      z,cbrtf_done
        ld      a,h
        xor     #0x80
        ld      h,a
        jr      cbrtf_done

cbrtf_ret_x:
        ld      e,CBR_XLO(ix)
        ld      d,CBR_XLO+1(ix)
        ld      l,CBR_XHI(ix)
        ld      h,CBR_XHI+1(ix)
cbrtf_done:
        ld      sp,ix
        pop     ix
        ret
