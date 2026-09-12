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
        ; preserves all registers; restores the outermost caller's IFF.
        ; An unmatched leave is a no-op, not an implicit EI.
_leave_critical_section::
        push    af
        push    hl
        ld      hl, #__interrupt_refcount
        ld      a,(hl)
        and     #0x7f
        jr      z, .done
        dec     (hl)
        ld      a,(hl)
        cp      #0x80
        jr      nz, .done
        ld      (hl),#0
        ei
.done:
        pop     hl
        pop     af
        ret
