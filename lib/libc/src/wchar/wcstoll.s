        ;; wcstoll.s
        ;;
        ;; Wide-string to long long conversion sharing the 64-bit wide parser.

        .module wcstoll
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoll
        .globl  __wcstox_core, __wsx_negate
        .globl  __errno_value
WSX_BUF .equ -9
WSX_FLG .equ -1

        .area   _CODE

_wcstoll::
        ld      c,l
        ld      b,h
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-9
        add     hl,sp
        ld      sp,hl
        ld      l,c
        ld      h,b
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        push    ix
        pop     iy
        ld      bc,#WSX_BUF
        add     iy,bc
        pop     bc
        call    __wcstox_core
        ld      WSX_FLG(ix),a
        bit     0,a
        jr      z,wcstoll_zero
        bit     2,a
        jp      nz,wcstoll_range
        ld      a,WSX_FLG(ix)
        bit     1,a
        jr      nz,wcstoll_neg
        ld      a,WSX_BUF + 7(ix)
        bit     7,a
        jp      nz,wcstoll_range_max
        jr      wcstoll_load
wcstoll_neg:
        ld      a,WSX_BUF + 7(ix)
        cp      #0x80
        jr      c,wcstoll_neg_ok
        jp      nz,wcstoll_range_min
        push    ix
        pop     hl
        ld      bc,#WSX_BUF
        add     hl,bc
        ld      b,#7
        xor     a
wcstoll_or:
        or      (hl)
        inc     hl
        djnz    wcstoll_or
        or      a
        jp      z,wcstoll_long_min
        jp      wcstoll_range_min
wcstoll_neg_ok:
        push    ix
        pop     hl
        ld      bc,#WSX_BUF
        add     hl,bc
        call    __wsx_negate
wcstoll_load:
        ld      e,WSX_BUF(ix)
        ld      d,WSX_BUF + 1(ix)
        ld      l,WSX_BUF + 2(ix)
        ld      h,WSX_BUF + 3(ix)
        exx
        ld      e,WSX_BUF + 4(ix)
        ld      d,WSX_BUF + 5(ix)
        ld      l,WSX_BUF + 6(ix)
        ld      h,WSX_BUF + 7(ix)
        exx
        ld      sp,ix
        pop     ix
        ret
wcstoll_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
        ld      sp,ix
        pop     ix
        ret
wcstoll_long_min:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0x8000
        exx
        ld      sp,ix
        pop     ix
        ret
wcstoll_range:
        ld      a,WSX_FLG(ix)
        bit     1,a
        jp      nz,wcstoll_range_min
wcstoll_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        exx
        ld      de,#0xffff
        ld      hl,#0x7fff
        exx
        ld      sp,ix
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
        ld      sp,ix
        pop     ix
        ret
