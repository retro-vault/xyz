        ; strtoll.s — string to long long (64-bit) via the shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtoll
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtoll
        .globl  __strtox_core, __sx_negate
        .globl  __errno_value
SX_BUF  .equ -9
SX_FLG  .equ -1
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL:DE':HL' = result
_strtoll::
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
        jr      z,sll_zero
        bit     2,a
        jp      nz,sll_range
        ld      a,SX_FLG(ix)
        bit     1,a
        jr      nz,sll_neg
        ; positive: bit63 set -> > LLONG_MAX -> range
        ld      a,SX_BUF + 7(ix)
        bit     7,a
        jp      nz,sll_range_max
        jr      sll_load
sll_neg:
        ; negative: value > 0x8000000000000000 -> range; == -> LLONG_MIN
        ld      a,SX_BUF + 7(ix)
        cp      #0x80
        jr      c,sll_neg_ok
        jp      nz,sll_range_min
        ; top byte == 0x80; lower 7 bytes nonzero -> > limit
        push    ix
        pop     hl
        ld      bc,#SX_BUF
        add     hl,bc
        ld      b,#7
        xor     a
sll_or:
        or      (hl)
        inc     hl
        djnz    sll_or
        or      a
        jp      z,sll_long_min          ; exactly 0x8000...000
        jp      sll_range_min
sll_neg_ok:
        push    ix
        pop     hl
        ld      bc,#SX_BUF
        add     hl,bc
        call    __sx_negate
sll_load:
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
sll_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
        ld      sp,ix
        pop     ix
        ret
sll_long_min:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0x8000              ; LLONG_MIN = 0x8000000000000000
        exx
        ld      sp,ix
        pop     ix
        ret
sll_range:
        ld      a,SX_FLG(ix)
        bit     1,a
        jp      nz,sll_range_min
sll_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        exx
        ld      de,#0xffff
        ld      hl,#0x7fff              ; LLONG_MAX
        exx
        ld      sp,ix
        pop     ix
        ret
sll_range_min:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0x8000              ; LLONG_MIN
        exx
        ld      sp,ix
        pop     ix
        ret
