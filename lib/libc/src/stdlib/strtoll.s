        ; strtoll.s — string to long long (64-bit) via the shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtoll
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtoll
        .globl  __strtox_core, __sx_negate
        .globl  __sx_acc, __sx_neg, __sx_ovf, __sx_any
        .globl  __errno_value
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL:DE':HL' = result
_strtoll::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __strtox_core
        ld      a,(__sx_any)
        or      a
        jr      z,sll_zero
        ld      a,(__sx_ovf)
        or      a
        jr      nz,sll_range
        ld      a,(__sx_neg)
        or      a
        jr      nz,sll_neg
        ; positive: bit63 set -> > LLONG_MAX -> range
        ld      a,(__sx_acc + 7)
        bit     7,a
        jr      nz,sll_range_max
        jr      sll_load
sll_neg:
        ; negative: value > 0x8000000000000000 -> range; == -> LLONG_MIN
        ld      a,(__sx_acc + 7)
        cp      #0x80
        jr      c,sll_neg_ok
        jr      nz,sll_range_min
        ; top byte == 0x80; lower 7 bytes nonzero -> > limit
        ld      hl,#__sx_acc
        ld      b,#7
        xor     a
sll_or:
        or      (hl)
        inc     hl
        djnz    sll_or
        or      a
        jr      z,sll_long_min          ; exactly 0x8000...000
        jr      sll_range_min
sll_neg_ok:
        call    __sx_negate
sll_load:
        ld      de,(__sx_acc)
        ld      hl,(__sx_acc + 2)
        exx
        ld      de,(__sx_acc + 4)
        ld      hl,(__sx_acc + 6)
        exx
        pop     ix
        ret
sll_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
        pop     ix
        ret
sll_long_min:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0x8000              ; LLONG_MIN = 0x8000000000000000
        exx
        pop     ix
        ret
sll_range:
        ld      a,(__sx_neg)
        or      a
        jr      nz,sll_range_min
sll_range_max:
        ld      hl,#34
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff
        exx
        ld      de,#0xffff
        ld      hl,#0x7fff              ; LLONG_MAX
        exx
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
        pop     ix
        ret
