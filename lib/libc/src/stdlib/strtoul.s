        ; strtoul.s — string to unsigned long (32-bit) via the shared parser.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module strtoul
        .optsdcc -mz80 sdcccall(1)
        .globl  _strtoul
        .globl  __strtox_core
        .globl  __sx_acc, __sx_neg, __sx_ovf, __sx_any
        .globl  __errno_value
        .area   _CODE
        ; HL = nptr, DE = endptr, 4(ix) = base -> DE:HL = result
_strtoul::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        call    __strtox_core
        ld      a,(__sx_any)
        or      a
        jr      z,sul_zero
        ld      a,(__sx_ovf)
        or      a
        jr      nz,sul_range
        ; high 32 bits nonzero -> out of unsigned-long range
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
        jr      nz,sul_range
        ld      de,(__sx_acc)
        ld      hl,(__sx_acc + 2)
        ld      a,(__sx_neg)
        or      a
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
        pop     ix
        ret
sul_zero:
        ld      de,#0
        ld      hl,#0
        pop     ix
        ret
sul_range:
        ld      hl,#34                  ; ERANGE
        ld      (__errno_value),hl
        ld      de,#0xffff
        ld      hl,#0xffff              ; ULONG_MAX
        pop     ix
        ret
