        ; strtol.s — string to long (32-bit) via the shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtol
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtol
        .globl  __strtox_core
        .globl  __errno_value
SX_BUF  .equ -9
SX_FLG  .equ -1
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL = result
_strtol::
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
        jr      z,stl_zero
        bit     2,a
        jp      nz,stl_range
        ; high 32 bits nonzero -> out of range
        ld      a,SX_BUF + 4(ix)
        ld      b,a
        ld      a,SX_BUF + 5(ix)
        or      b
        ld      b,a
        ld      a,SX_BUF + 6(ix)
        or      b
        ld      b,a
        ld      a,SX_BUF + 7(ix)
        or      b
        jp      nz,stl_range
        ld      a,SX_FLG(ix)
        bit     1,a
        jr      nz,stl_neg
        ; positive: value > 0x7FFFFFFF ? (bit31 set)
        ld      a,SX_BUF + 3(ix)
        bit     7,a
        jp      nz,stl_range_max
        ld      e,SX_BUF(ix)
        ld      d,SX_BUF + 1(ix)
        ld      l,SX_BUF + 2(ix)
        ld      h,SX_BUF + 3(ix)
        ld      sp,ix
        pop     ix
        ret
stl_neg:
        ld      a,SX_BUF + 3(ix)
        cp      #0x80
        jr      c,stl_neg_ok            ; < 0x80000000
        jp      nz,stl_range_min        ; > 0x80xxxxxx
        ld      a,SX_BUF(ix)
        ld      b,a
        ld      a,SX_BUF + 1(ix)
        or      b
        ld      b,a
        ld      a,SX_BUF + 2(ix)
        or      b
        jp      z,stl_long_min          ; exactly 0x80000000 -> LONG_MIN
        jp      stl_range_min
stl_neg_ok:
        ld      e,SX_BUF(ix)
        ld      d,SX_BUF + 1(ix)
        ld      l,SX_BUF + 2(ix)
        ld      h,SX_BUF + 3(ix)
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
        jr      nz,stl_ret
        inc     hl
stl_ret:
        ld      sp,ix
        pop     ix
        ret
stl_zero:
        ld      de,#0
        ld      hl,#0
        ld      sp,ix
        pop     ix
        ret
stl_long_min:
        ld      de,#0x0000
        ld      hl,#0x8000              ; LONG_MIN
        ld      sp,ix
        pop     ix
        ret
stl_range:
        ld      a,SX_FLG(ix)
        bit     1,a
        jp      nz,stl_range_min
stl_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0x7fff              ; LONG_MAX
        ld      sp,ix
        pop     ix
        ret
stl_range_min:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0x0000
        ld      hl,#0x8000              ; LONG_MIN
        ld      sp,ix
        pop     ix
        ret
