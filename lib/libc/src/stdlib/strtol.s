        ; strtol.s — string to long (32-bit) via the shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtol
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtol
        .globl  __strtox_core
        .globl  __sx_acc, __sx_neg, __sx_ovf, __sx_any
        .globl  __errno_value
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL = result
_strtol::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __strtox_core
        ld      a,(__sx_any)
        or      a
        jr      z,stl_zero
        ld      a,(__sx_ovf)
        or      a
        jr      nz,stl_range
        ; high 32 bits nonzero -> out of range
        ld      a,(__sx_acc + 4)
        ld      b,a
        ld      a,(__sx_acc + 5)
        or      b
        ld      b,a
        ld      a,(__sx_acc + 6)
        or      b
        ld      b,a
        ld      a,(__sx_acc + 7)
        or      b
        jr      nz,stl_range
        ld      a,(__sx_neg)
        or      a
        jr      nz,stl_neg
        ; positive: value > 0x7FFFFFFF ? (bit31 set)
        ld      a,(__sx_acc + 3)
        bit     7,a
        jr      nz,stl_range_max
        ld      de,(__sx_acc)
        ld      hl,(__sx_acc + 2)
        pop     ix
        ret
stl_neg:
        ld      a,(__sx_acc + 3)
        cp      #0x80
        jr      c,stl_neg_ok            ; < 0x80000000
        jr      nz,stl_range_min        ; > 0x80xxxxxx
        ld      a,(__sx_acc)
        ld      b,a
        ld      a,(__sx_acc + 1)
        or      b
        ld      b,a
        ld      a,(__sx_acc + 2)
        or      b
        jr      z,stl_long_min          ; exactly 0x80000000 -> LONG_MIN
        jr      stl_range_min
stl_neg_ok:
        ld      de,(__sx_acc)
        ld      hl,(__sx_acc + 2)
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
        pop     ix
        ret
stl_zero:
        ld      de,#0
        ld      hl,#0
        pop     ix
        ret
stl_long_min:
        ld      de,#0x0000
        ld      hl,#0x8000              ; LONG_MIN
        pop     ix
        ret
stl_range:
        ld      a,(__sx_neg)
        or      a
        jr      nz,stl_range_min
stl_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0x7fff              ; LONG_MAX
        pop     ix
        ret
stl_range_min:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0x0000
        ld      hl,#0x8000              ; LONG_MIN
        pop     ix
        ret
