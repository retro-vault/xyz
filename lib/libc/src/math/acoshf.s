        ;; acoshf.s
        ;;
        ;; libc acoshf for the xcc Z80 libc.
        ;;
        ;; Use
        ;;   acosh(x) = log(x + sqrt((x - 1) * (x + 1)))
        ;; for the valid domain x >= 1. Inputs below 1 return a quiet NaN.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module acoshf
        .optsdcc -mz80 sdcccall(1)

        .globl  _acoshf
        .globl  ___libc_fpclassifyf
        .globl  ___fscmp
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsmul
        .globl  _sqrtf
        .globl  _logf

        .area   _DATA
__acoshf_x:      .ds 4
__acoshf_xm1:    .ds 4
__acoshf_tmp:    .ds 4

        .area   _CODE

_acoshf::
        ld      (__acoshf_x),de
        ld      (__acoshf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> preserve payload/sign
        jp      z,acoshf_ret_x
        cp      #1                      ; +Inf -> log(+Inf)
        jp      z,acoshf_ret_x

        ;; Reject x < 1 and return +0 exactly for x == 1.
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__acoshf_x)
        ld      hl,(__acoshf_x + 2)
        call    ___fscmp
        pop     bc
        pop     bc
        ld      a,d
        cp      #0xff
        jp      z,acoshf_ret_nan
        ld      a,d
        or      e
        jp      z,acoshf_ret_zero

        ;; xm1 = x - 1
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__acoshf_x)
        ld      hl,(__acoshf_x + 2)
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__acoshf_xm1),de
        ld      (__acoshf_xm1 + 2),hl

        ;; tmp = x + 1
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__acoshf_x)
        ld      hl,(__acoshf_x + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__acoshf_tmp),de
        ld      (__acoshf_tmp + 2),hl

        ;; tmp = (x - 1) * (x + 1)
        ld      hl,(__acoshf_tmp + 2)
        push    hl
        ld      hl,(__acoshf_tmp)
        push    hl
        ld      de,(__acoshf_xm1)
        ld      hl,(__acoshf_xm1 + 2)
        call    ___fsmul
        pop     bc
        pop     bc

        ;; tmp = sqrt((x - 1) * (x + 1))
        call    _sqrtf
        ld      (__acoshf_tmp),de
        ld      (__acoshf_tmp + 2),hl

        ;; tmp = x + sqrt((x - 1) * (x + 1))
        ld      hl,(__acoshf_tmp + 2)
        push    hl
        ld      hl,(__acoshf_tmp)
        push    hl
        ld      de,(__acoshf_x)
        ld      hl,(__acoshf_x + 2)
        call    ___fsadd
        pop     bc
        pop     bc

        jp      _logf

acoshf_ret_zero:
        ld      hl,#0x0000
        ld      de,#0x0000
        ret

acoshf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ret

acoshf_ret_x:
        ld      de,(__acoshf_x)
        ld      hl,(__acoshf_x + 2)
        ret
