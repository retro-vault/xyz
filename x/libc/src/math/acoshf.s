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

        .area   _CODE

_acoshf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),c
        ld      -9(ix),b                ; x
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
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
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        call    ___fscmp
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
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h                ; xm1

        ;; tmp = x + 1
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h                ; tmp

        ;; tmp = (x - 1) * (x + 1)
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    ___fsmul
        pop     bc
        pop     bc

        ;; tmp = sqrt((x - 1) * (x + 1))
        call    _sqrtf
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h

        ;; tmp = x + sqrt((x - 1) * (x + 1))
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        call    ___fsadd
        pop     bc
        pop     bc

        ld      sp,ix
        pop     ix
        jp      _logf

acoshf_ret_zero:
        ld      hl,#0x0000
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret

acoshf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret

acoshf_ret_x:
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        ld      sp,ix
        pop     ix
        ret
