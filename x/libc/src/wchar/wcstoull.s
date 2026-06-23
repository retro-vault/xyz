        ;; wcstoull.s
        ;;
        ;; Wide-string to unsigned long long conversion.

        .module wcstoull
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoull
        .globl  __wcstox_core, __wsx_negate
        .globl  __errno_value
WSX_BUF .equ -9
WSX_FLG .equ -1

        .area   _CODE

_wcstoull::
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
        jr      z,wcstoull_zero
        bit     2,a
        jp      nz,wcstoull_range
        ld      a,WSX_FLG(ix)
        bit     1,a
        jr      z,wcstoull_load
        push    ix
        pop     hl
        ld      bc,#WSX_BUF
        add     hl,bc
        call    __wsx_negate
wcstoull_load:
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
wcstoull_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
        ld      sp,ix
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
        ld      sp,ix
        pop     ix
        ret
