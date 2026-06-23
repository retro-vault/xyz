        ; strtoull.s — string to unsigned long long (64-bit) via shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtoull
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtoull
        .globl  __strtox_core, __sx_negate
        .globl  __errno_value
SX_BUF  .equ -9
SX_FLG  .equ -1
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL:DE':HL' = result
_strtoull::
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
        ld      bc,#SX_BUF
        add     iy,bc
        pop     bc
        call    __strtox_core
        ld      SX_FLG(ix),a
        bit     0,a
        jr      z,sull_zero
        bit     2,a
        jp      nz,sull_range
        ld      a,SX_FLG(ix)
        bit     1,a
        jr      z,sull_load
        push    ix
        pop     hl
        ld      bc,#SX_BUF
        add     hl,bc
        call    __sx_negate
sull_load:
        ld      e,SX_BUF(ix)
        ld      d,SX_BUF + 1(ix)
        ld      l,SX_BUF + 2(ix)
        ld      h,SX_BUF + 3(ix)
        exx
        ld      e,SX_BUF + 4(ix)
        ld      d,SX_BUF + 5(ix)
        ld      l,SX_BUF + 6(ix)
        ld      h,SX_BUF + 7(ix)
        exx
        ld      sp,ix
        pop     ix
        ret
sull_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
        ld      sp,ix
        pop     ix
        ret
sull_range:
        ld      hl,#34                  ; ERANGE
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        exx
        ld      de,#0xffff
        ld      hl,#0xffff              ; ULLONG_MAX
        exx
        ld      sp,ix
        pop     ix
        ret
