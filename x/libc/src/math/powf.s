        ;; powf.s
        ;;
        ;; Computes x^y as exp(y * log(x)) for positive bases. Negative bases
        ;; are accepted only when y is an integer; in that case the magnitude
        ;; path uses log(|x|) and the final sign is restored for odd exponents.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module powf
        .optsdcc -mz80 sdcccall(1)

        .globl  _powf
        .globl  _truncf
        .globl  ___fsmul
        .globl  ___fscmp
        .globl  ___fs2slong
        .globl  __libc_logf_core
        .globl  __libc_expf_core

        .area   _CODE

POW_XLO  .equ -13
POW_XHI  .equ -11
POW_YLO  .equ -9
POW_YHI  .equ -7
POW_TLO  .equ -5
POW_THI  .equ -3
POW_ODD  .equ -1

_powf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-13
        add     hl,sp
        ld      sp,hl
        ld      POW_XLO(ix),e
        ld      POW_XLO+1(ix),d
        ld      POW_XHI(ix),c
        ld      POW_XHI+1(ix),b
        ld      a,4(ix)
        ld      POW_YLO(ix),a
        ld      a,5(ix)
        ld      POW_YLO+1(ix),a
        ld      a,6(ix)
        ld      POW_YHI(ix),a
        ld      a,7(ix)
        ld      POW_YHI+1(ix),a

        ;; Compare x with 0.
        ld      hl,#0x0000
        push    hl
        push    hl
        ld      e,POW_XLO(ix)
        ld      d,POW_XLO+1(ix)
        ld      l,POW_XHI(ix)
        ld      h,POW_XHI+1(ix)
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jp      z,powf_neg_base
        ld      a,d
        or      e
        jr      z,powf_zero_base

        ;; Positive base: exp(y * log(x))
powf_pos_base:
        ld      e,POW_XLO(ix)
        ld      d,POW_XLO+1(ix)
        ld      l,POW_XHI(ix)
        ld      h,POW_XHI+1(ix)
        call    __libc_logf_core
        ld      POW_TLO(ix),e
        ld      POW_TLO+1(ix),d
        ld      POW_THI(ix),l
        ld      POW_THI+1(ix),h
        ld      l,POW_YHI(ix)
        ld      h,POW_YHI+1(ix)
        push    hl
        ld      e,POW_YLO(ix)
        ld      d,POW_YLO+1(ix)
        push    de
        ld      e,POW_TLO(ix)
        ld      d,POW_TLO+1(ix)
        ld      l,POW_THI(ix)
        ld      h,POW_THI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        call    __libc_expf_core
        ld      sp,ix
        pop     ix
        ret

powf_zero_base:
        ;; 0^0 -> 1, 0^positive -> 0, 0^negative -> +Inf.
        ld      hl,#0x0000
        push    hl
        push    hl
        ld      e,POW_YLO(ix)
        ld      d,POW_YLO+1(ix)
        ld      l,POW_YHI(ix)
        ld      h,POW_YHI+1(ix)
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      z,powf_zero_neg
        ld      a,d
        or      e
        jr      z,powf_zero_zero
        ld      hl,#0x0000
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
powf_zero_zero:
        ld      hl,#0x3f80
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
powf_zero_neg:
        ld      hl,#0x7f80
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret

powf_neg_base:
        ;; Accept negative bases only for integral exponents.
        ld      e,POW_YLO(ix)
        ld      d,POW_YLO+1(ix)
        ld      l,POW_YHI(ix)
        ld      h,POW_YHI+1(ix)
        call    _truncf
        ld      POW_TLO(ix),e
        ld      POW_TLO+1(ix),d
        ld      POW_THI(ix),l
        ld      POW_THI+1(ix),h

        ld      l,POW_YHI(ix)
        ld      h,POW_YHI+1(ix)
        push    hl
        ld      e,POW_YLO(ix)
        ld      d,POW_YLO+1(ix)
        push    de
        ld      e,POW_TLO(ix)
        ld      d,POW_TLO+1(ix)
        ld      l,POW_THI(ix)
        ld      h,POW_THI+1(ix)
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      z,powf_domain
        ld      a,d
        or      e
        jr      nz,powf_domain

        ;; Record whether the integral exponent is odd.
        ld      e,POW_TLO(ix)
        ld      d,POW_TLO+1(ix)
        ld      l,POW_THI(ix)
        ld      h,POW_THI+1(ix)
        call    ___fs2slong
        ld      a,e
        and     #1
        ld      POW_ODD(ix),a

        ;; log(|x|), then exp(y * log(|x|)).
        ld      e,POW_XLO(ix)
        ld      d,POW_XLO+1(ix)
        ld      l,POW_XHI(ix)
        ld      h,POW_XHI+1(ix)
        ld      a,h
        xor     #0x80
        ld      h,a
        call    __libc_logf_core
        ld      POW_TLO(ix),e
        ld      POW_TLO+1(ix),d
        ld      POW_THI(ix),l
        ld      POW_THI+1(ix),h
        ld      l,POW_YHI(ix)
        ld      h,POW_YHI+1(ix)
        push    hl
        ld      e,POW_YLO(ix)
        ld      d,POW_YLO+1(ix)
        push    de
        ld      e,POW_TLO(ix)
        ld      d,POW_TLO+1(ix)
        ld      l,POW_THI(ix)
        ld      h,POW_THI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        call    __libc_expf_core
        ld      a,POW_ODD(ix)
        or      a
        jr      z,powf_ret
        ld      a,h
        xor     #0x80
        ld      h,a
powf_ret:
        ld      sp,ix
        pop     ix
        ret

powf_domain:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
