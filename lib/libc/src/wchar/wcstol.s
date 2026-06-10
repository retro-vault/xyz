        ;; wcstol.s
        ;;
        ;; Wide-string to long conversion. The helper parses byte-range wchar_t
        ;; code units with the same syntax and range rules as strtol().

        .module wcstol
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstol
        .globl  __wcstox_core
        .globl  __wsx_acc, __wsx_neg, __wsx_ovf, __wsx_any
        .globl  __errno_value

        .area   _CODE

_wcstol::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __wcstox_core
        ld      a,(__wsx_any)
        or      a
        jr      z,wcstol_zero
        ld      a,(__wsx_ovf)
        or      a
        jr      nz,wcstol_range
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
        jr      nz,wcstol_range
        ld      a,(__wsx_neg)
        or      a
        jr      nz,wcstol_neg
        ld      a,(__wsx_acc + 3)
        bit     7,a
        jr      nz,wcstol_range_max
        ld      de,(__wsx_acc)
        ld      hl,(__wsx_acc + 2)
        pop     ix
        ret
wcstol_neg:
        ld      a,(__wsx_acc + 3)
        cp      #0x80
        jr      c,wcstol_neg_ok
        jr      nz,wcstol_range_min
        ld      a,(__wsx_acc)
        ld      b,a
        ld      a,(__wsx_acc + 1)
        or      b
        ld      b,a
        ld      a,(__wsx_acc + 2)
        or      b
        jr      z,wcstol_long_min
        jr      wcstol_range_min
wcstol_neg_ok:
        ld      de,(__wsx_acc)
        ld      hl,(__wsx_acc + 2)
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
        jr      nz,wcstol_ret
        inc     hl
wcstol_ret:
        pop     ix
        ret
wcstol_zero:
        ld      de,#0
        ld      hl,#0
        pop     ix
        ret
wcstol_long_min:
        ld      de,#0x0000
        ld      hl,#0x8000
        pop     ix
        ret
wcstol_range:
        ld      a,(__wsx_neg)
        or      a
        jr      nz,wcstol_range_min
wcstol_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0x7fff
        pop     ix
        ret
wcstol_range_min:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0x0000
        ld      hl,#0x8000
        pop     ix
        ret
