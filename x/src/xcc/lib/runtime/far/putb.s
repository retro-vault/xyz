        ; Far-pointer write trampoline (default / unbanked target).
        ;
        ; A far pointer is 24 bits: C = bank, HL = 16-bit address.  This
        ; default implementation (used by the 'none' and 'cpm3' targets,
        ; which have no bank switching) ignores the bank and performs a
        ; plain memory write.  A banked target overrides __far_putb by
        ; linking its own module ahead of the runtime library.
        ;
        ; Contract: BC, DE and HL MUST be preserved so the compiler can
        ; keep the far pointer live across consecutive byte accesses.
        ; A holds the byte to store and may change.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module far_putb
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __far_putb

        ; __far_putb
        ; inputs:  HL = address, C = bank, A = byte to store
        ; outputs: stores A at (bank:HL)
        ; clobbers: F

__far_putb:
        ld      (hl), a
        ret
