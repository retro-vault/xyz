        ; Far-pointer read trampoline (default / unbanked target).
        ;
        ; A far pointer is 24 bits: C = bank, HL = 16-bit address.  This
        ; default implementation (used by the 'none' and 'cpm3' targets,
        ; which have no bank switching) ignores the bank and performs a
        ; plain memory read.  A banked target overrides __far_getb by
        ; linking its own module ahead of the runtime library.
        ;
        ; Contract: only A and F may change.  BC, DE and HL MUST be
        ; preserved so the compiler can keep the far pointer live across
        ; consecutive byte accesses.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module far_getb
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __far_getb

        ; __far_getb
        ; inputs:  HL = address, C = bank
        ; outputs: A  = byte at (bank:HL)
        ; clobbers: A, F

__far_getb:
        ld      a, (hl)
        ret
