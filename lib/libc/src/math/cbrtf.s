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

        .area   _DATA
__cbrtf_sign:
        .ds     1
__cbrtf_x:
        .ds     4

        .area   _CODE

_cbrtf::
        ld      (__cbrtf_x),de
        ld      (__cbrtf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> return unchanged
        jr      z,cbrtf_ret_x
        cp      #1                      ; Inf -> return unchanged
        jr      z,cbrtf_ret_x
        cp      #2                      ; Zero -> preserve signed zero
        jr      z,cbrtf_ret_x

        ld      de,(__cbrtf_x)
        ld      hl,(__cbrtf_x + 2)
        call    ___libc_signbitf
        ld      a,d
        or      e
        ld      a,#0
        jr      z,cbrtf_have_sign
        ld      a,#1
cbrtf_have_sign:
        ld      (__cbrtf_sign),a

        ld      de,(__cbrtf_x)
        ld      hl,(__cbrtf_x + 2)
        ld      a,(__cbrtf_sign)
        or      a
        jr      z,cbrtf_positive
        ld      a,h
        xor     #0x80
        ld      h,a
cbrtf_positive:
        call    __libc_logf_core
        ld      (__cbrtf_x),de
        ld      (__cbrtf_x + 2),hl
        ld      hl,#0x3eaa              ; 1/3
        push    hl
        ld      hl,#0xaaab
        push    hl
        ld      de,(__cbrtf_x)
        ld      hl,(__cbrtf_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        call    __libc_expf_core
        ld      a,(__cbrtf_sign)
        or      a
        ret     z
        ld      a,h
        xor     #0x80
        ld      h,a
        ret

cbrtf_ret_x:
        ld      de,(__cbrtf_x)
        ld      hl,(__cbrtf_x + 2)
        ret
