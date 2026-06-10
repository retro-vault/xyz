        ;; wcstoll.s
        ;;
        ;; Wide-string to long long conversion sharing the 64-bit wide parser.

        .module wcstoll
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoll
        .globl  __wcstox_core, __wsx_negate
        .globl  __wsx_acc, __wsx_neg, __wsx_ovf, __wsx_any
        .globl  __errno_value

        .area   _CODE

_wcstoll::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __wcstox_core
        ld      a,(__wsx_any)
        or      a
        jr      z,wcstoll_zero
        ld      a,(__wsx_ovf)
        or      a
        jr      nz,wcstoll_range
        ld      a,(__wsx_neg)
        or      a
        jr      nz,wcstoll_neg
        ld      a,(__wsx_acc + 7)
        bit     7,a
        jr      nz,wcstoll_range_max
        jr      wcstoll_load
wcstoll_neg:
        ld      a,(__wsx_acc + 7)
        cp      #0x80
        jr      c,wcstoll_neg_ok
        jr      nz,wcstoll_range_min
        ld      hl,#__wsx_acc
        ld      b,#7
        xor     a
wcstoll_or:
        or      (hl)
        inc     hl
        djnz    wcstoll_or
        or      a
        jr      z,wcstoll_long_min
        jr      wcstoll_range_min
wcstoll_neg_ok:
        call    __wsx_negate
wcstoll_load:
        ld      de,(__wsx_acc)
        ld      hl,(__wsx_acc + 2)
        exx
        ld      de,(__wsx_acc + 4)
        ld      hl,(__wsx_acc + 6)
        exx
        pop     ix
        ret
wcstoll_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
        pop     ix
        ret
wcstoll_long_min:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0x8000
        exx
        pop     ix
        ret
wcstoll_range:
        ld      a,(__wsx_neg)
        or      a
        jr      nz,wcstoll_range_min
wcstoll_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        exx
        ld      de,#0xffff
        ld      hl,#0x7fff
        exx
        pop     ix
        ret
wcstoll_range_min:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0x8000
        exx
        pop     ix
        ret
