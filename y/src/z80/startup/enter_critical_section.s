        ; Enter a nestable interrupt-disabled critical section.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module enter_critical_section
        .optsdcc -mz80 sdcccall(1)

        .globl  _enter_critical_section
        .globl  __interrupt_refcount

        .area   _CODE

        ; _enter_critical_section, sdcccall(1)
        ; outputs: none
        ; preserves all registers, including flags; at most 127 levels.
        ; Bit 7 remembers the outermost caller's interrupt-enable state.
_enter_critical_section::
        push    af
        push    hl
        ld      a,i
        di
        ld      hl, #__interrupt_refcount
        jp      po, .disabled
        set     7,(hl)
.disabled:
        inc     (hl)
        pop     hl
        pop     af
        ret
