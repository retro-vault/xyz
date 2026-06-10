        ;; wcstoull.s
        ;;
        ;; Wide-string to unsigned long long conversion.

        .module wcstoull
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoull
        .globl  __wcstox_core, __wsx_negate
        .globl  __wsx_acc, __wsx_neg, __wsx_ovf, __wsx_any
        .globl  __errno_value

        .area   _CODE

_wcstoull::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __wcstox_core
        ld      a,(__wsx_any)
        or      a
        jr      z,wcstoull_zero
        ld      a,(__wsx_ovf)
        or      a
        jr      nz,wcstoull_range
        ld      a,(__wsx_neg)
        or      a
        jr      z,wcstoull_load
        call    __wsx_negate
wcstoull_load:
        ld      de,(__wsx_acc)
        ld      hl,(__wsx_acc + 2)
        exx
        ld      de,(__wsx_acc + 4)
        ld      hl,(__wsx_acc + 6)
        exx
        pop     ix
        ret
wcstoull_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
        pop     ix
        ret
wcstoull_range:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        exx
        ld      de,#0xffff
        ld      hl,#0xffff
        exx
        pop     ix
        ret
