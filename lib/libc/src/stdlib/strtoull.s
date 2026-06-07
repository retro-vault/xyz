        ; strtoull.s — string to unsigned long long (64-bit) via shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtoull
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtoull
        .globl  __strtox_core, __sx_negate
        .globl  __sx_acc, __sx_neg, __sx_ovf, __sx_any
        .globl  __errno_value
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL:DE':HL' = result
_strtoull::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __strtox_core
        ld      a,(__sx_any)
        or      a
        jr      z,sull_zero
        ld      a,(__sx_ovf)
        or      a
        jr      nz,sull_range
        ld      a,(__sx_neg)
        or      a
        jr      z,sull_load
        call    __sx_negate
sull_load:
        ld      de,(__sx_acc)
        ld      hl,(__sx_acc + 2)
        exx
        ld      de,(__sx_acc + 4)
        ld      hl,(__sx_acc + 6)
        exx
        pop     ix
        ret
sull_zero:
        ld      de,#0
        ld      hl,#0
        exx
        ld      de,#0
        ld      hl,#0
        exx
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
        pop     ix
        ret
