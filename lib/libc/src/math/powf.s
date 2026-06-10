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

        .area   _DATA
__powf_x:
        .ds     4
__powf_y:
        .ds     4
__powf_t:
        .ds     4
__powf_odd:
        .ds     1

        .area   _CODE

_powf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__powf_x),de
        ld      (__powf_x + 2),hl
        ld      a,4(ix)
        ld      (__powf_y),a
        ld      a,5(ix)
        ld      (__powf_y + 1),a
        ld      a,6(ix)
        ld      (__powf_y + 2),a
        ld      a,7(ix)
        ld      (__powf_y + 3),a

        ;; Compare x with 0.
        ld      hl,#0x0000
        push    hl
        push    hl
        ld      de,(__powf_x)
        ld      hl,(__powf_x + 2)
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      z,powf_neg_base
        ld      a,d
        or      e
        jr      z,powf_zero_base

        ;; Positive base: exp(y * log(x))
powf_pos_base:
        ld      de,(__powf_x)
        ld      hl,(__powf_x + 2)
        call    __libc_logf_core
        ld      (__powf_t),de
        ld      (__powf_t + 2),hl
        ld      hl,(__powf_y + 2)
        push    hl
        ld      hl,(__powf_y)
        push    hl
        ld      de,(__powf_t)
        ld      hl,(__powf_t + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        call    __libc_expf_core
        pop     ix
        ret

powf_zero_base:
        ;; 0^0 -> 1, 0^positive -> 0, 0^negative -> +Inf.
        ld      hl,#0x0000
        push    hl
        push    hl
        ld      de,(__powf_y)
        ld      hl,(__powf_y + 2)
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      z,powf_zero_neg
        ld      a,d
        or      e
        jr      z,powf_zero_zero
        ld      hl,#0x0000
        ld      de,#0x0000
        pop     ix
        ret
powf_zero_zero:
        ld      hl,#0x3f80
        ld      de,#0x0000
        pop     ix
        ret
powf_zero_neg:
        ld      hl,#0x7f80
        ld      de,#0x0000
        pop     ix
        ret

powf_neg_base:
        ;; Accept negative bases only for integral exponents.
        ld      de,(__powf_y)
        ld      hl,(__powf_y + 2)
        call    _truncf
        ld      (__powf_t),de
        ld      (__powf_t + 2),hl

        ld      hl,(__powf_y + 2)
        push    hl
        ld      hl,(__powf_y)
        push    hl
        ld      de,(__powf_t)
        ld      hl,(__powf_t + 2)
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      z,powf_domain
        ld      a,d
        or      e
        jr      nz,powf_domain

        ;; Record whether the integral exponent is odd.
        ld      de,(__powf_t)
        ld      hl,(__powf_t + 2)
        call    ___fs2slong
        ld      a,e
        and     #1
        ld      (__powf_odd),a

        ;; log(|x|), then exp(y * log(|x|)).
        ld      de,(__powf_x)
        ld      hl,(__powf_x + 2)
        ld      a,h
        xor     #0x80
        ld      h,a
        call    __libc_logf_core
        ld      (__powf_t),de
        ld      (__powf_t + 2),hl
        ld      hl,(__powf_y + 2)
        push    hl
        ld      hl,(__powf_y)
        push    hl
        ld      de,(__powf_t)
        ld      hl,(__powf_t + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        call    __libc_expf_core
        ld      a,(__powf_odd)
        or      a
        jr      z,powf_ret
        ld      a,h
        xor     #0x80
        ld      h,a
powf_ret:
        pop     ix
        ret

powf_domain:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        pop     ix
        ret
