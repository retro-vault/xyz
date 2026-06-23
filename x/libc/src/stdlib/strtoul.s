        ; strtoul.s — string to unsigned long (32-bit) via the shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtoul
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtoul
        .globl  __strtox_core
        .globl  __errno_value
SX_BUF  .equ -9
SX_FLG  .equ -1
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL = result
_strtoul::
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
        jr      z,sul_zero
        bit     2,a
        jp      nz,sul_range
        ; high 32 bits nonzero -> out of unsigned-long range
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
        jp      nz,sul_range
        ld      e,SX_BUF(ix)
        ld      d,SX_BUF + 1(ix)
        ld      l,SX_BUF + 2(ix)
        ld      h,SX_BUF + 3(ix)
        ld      a,SX_FLG(ix)
        bit     1,a
        jr      z,sul_ret
        ; negate 32-bit (0 - value)
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
        jr      nz,sul_ret
        inc     hl
sul_ret:
        ld      sp,ix
        pop     ix
        ret
sul_zero:
        ld      de,#0
        ld      hl,#0
        ld      sp,ix
        pop     ix
        ret
sul_range:
        ld      hl,#34                  ; ERANGE
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff              ; ULONG_MAX
        ld      sp,ix
        pop     ix
        ret
