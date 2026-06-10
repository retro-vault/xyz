        ;; wcstoul.s
        ;;
        ;; Wide-string to unsigned long conversion. Negative inputs still apply
        ;; the standard modulo wrap after the shared parser records the sign.

        .module wcstoul
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoul
        .globl  __wcstox_core
        .globl  __wsx_acc, __wsx_neg, __wsx_ovf, __wsx_any
        .globl  __errno_value

        .area   _CODE

_wcstoul::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __wcstox_core
        ld      a,(__wsx_any)
        or      a
        jr      z,wcstoul_zero
        ld      a,(__wsx_ovf)
        or      a
        jr      nz,wcstoul_range
        ld      a,(__wsx_acc + 4)
        ld      b,a
        ld      a,(__wsx_acc + 5)
        or      b
        ld      b,a
        ld      a,(__wsx_acc + 6)
        or      b
        ld      b,a
        ld      a,(__wsx_acc + 7)
        or      b
        jr      nz,wcstoul_range
        ld      de,(__wsx_acc)
        ld      hl,(__wsx_acc + 2)
        ld      a,(__wsx_neg)
        or      a
        jr      z,wcstoul_ret
        ld      a,e
        cpl
        ld      e,a
        ld      a,d
        cpl
        ld      d,a
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     de
        ld      a,d
        or      e
        jr      nz,wcstoul_ret
        inc     hl
wcstoul_ret:
        pop     ix
        ret
wcstoul_zero:
        ld      de,#0
        ld      hl,#0
        pop     ix
        ret
wcstoul_range:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        pop     ix
        ret
