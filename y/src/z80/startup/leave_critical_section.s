        ; Leave a nestable interrupt-disabled critical section.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module leave_critical_section
        .optsdcc -mz80 sdcccall(1)

        .globl  _leave_critical_section
        .globl  __interrupt_refcount

        .area   _CODE

        ; _leave_critical_section, sdcccall(1)
        ; outputs: none
        ; preserves all registers
_leave_critical_section::
        push    af
        ld      a, (__interrupt_refcount)
        or      a
        jr      z, .enable
        dec     a
        ld      (__interrupt_refcount), a
        or      a
        jr      nz, .done
.enable:
        ei
.done:
        pop     af
        ret
