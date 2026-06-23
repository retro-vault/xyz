        ;; wcstol.s
        ;;
        ;; Wide-string to long conversion. The helper parses byte-range wchar_t
        ;; code units with the same syntax and range rules as strtol().

        .module wcstol
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstol
        .globl  __wcstox_core
        .globl  __errno_value
WSX_BUF .equ -9
WSX_FLG .equ -1

        .area   _CODE

_wcstol::
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
        jr      z,wcstol_zero
        bit     2,a
        jp      nz,wcstol_range
        ld      a,WSX_BUF + 4(ix)
        ld      b,a
        ld      a,WSX_BUF + 5(ix)
        or      b
        ld      b,a
        ld      a,WSX_BUF + 6(ix)
        or      b
        ld      b,a
        ld      a,WSX_BUF + 7(ix)
        or      b
        jp      nz,wcstol_range
        ld      a,WSX_FLG(ix)
        bit     1,a
        jr      nz,wcstol_neg
        ld      a,WSX_BUF + 3(ix)
        bit     7,a
        jp      nz,wcstol_range_max
        ld      e,WSX_BUF(ix)
        ld      d,WSX_BUF + 1(ix)
        ld      l,WSX_BUF + 2(ix)
        ld      h,WSX_BUF + 3(ix)
        ld      sp,ix
        pop     ix
        ret
wcstol_neg:
        ld      a,WSX_BUF + 3(ix)
        cp      #0x80
        jr      c,wcstol_neg_ok
        jp      nz,wcstol_range_min
        ld      a,WSX_BUF(ix)
        ld      b,a
        ld      a,WSX_BUF + 1(ix)
        or      b
        ld      b,a
        ld      a,WSX_BUF + 2(ix)
        or      b
        jp      z,wcstol_long_min
        jp      wcstol_range_min
wcstol_neg_ok:
        ld      e,WSX_BUF(ix)
        ld      d,WSX_BUF + 1(ix)
        ld      l,WSX_BUF + 2(ix)
        ld      h,WSX_BUF + 3(ix)
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
        ld      sp,ix
        pop     ix
        ret
wcstol_zero:
        ld      de,#0
        ld      hl,#0
        ld      sp,ix
        pop     ix
        ret
wcstol_long_min:
        ld      de,#0x0000
        ld      hl,#0x8000
        ld      sp,ix
        pop     ix
        ret
wcstol_range:
        ld      a,WSX_FLG(ix)
        bit     1,a
        jp      nz,wcstol_range_min
wcstol_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0x7fff
        ld      sp,ix
        pop     ix
        ret
wcstol_range_min:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0x0000
        ld      hl,#0x8000
        ld      sp,ix
        pop     ix
        ret
