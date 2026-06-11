        ;; wcstoul.s
        ;;
        ;; Wide-string to unsigned long conversion. Negative inputs still apply
        ;; the standard modulo wrap after the shared parser records the sign.

        .module wcstoul
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstoul
        .globl  __wcstox_core
        .globl  __errno_value
WSX_BUF .equ -9
WSX_FLG .equ -1

        .area   _CODE

_wcstoul::
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
        jr      z,wcstoul_zero
        bit     2,a
        jp      nz,wcstoul_range
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
        jp      nz,wcstoul_range
        ld      e,WSX_BUF(ix)
        ld      d,WSX_BUF + 1(ix)
        ld      l,WSX_BUF + 2(ix)
        ld      h,WSX_BUF + 3(ix)
        ld      a,WSX_FLG(ix)
        bit     1,a
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
        ld      sp,ix
        pop     ix
        ret
wcstoul_zero:
        ld      de,#0
        ld      hl,#0
        ld      sp,ix
        pop     ix
        ret
wcstoul_range:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        ld      sp,ix
        pop     ix
        ret
