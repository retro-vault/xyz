        ; YOS far-data helpers. The RST handler executes in common ROM while
        ; the requested data bank temporarily replaces the caller's bank.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module yos_far
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __far_getb
        .globl  __far_putb

        ; HL = address, C = bank -> A = byte. Preserves BC/DE/HL.
__far_getb::
        or      a                       ; clear carry: read
        rst     0x30
        ret

        ; HL = address, C = bank, A = byte. Preserves BC/DE/HL.
__far_putb::
        scf                             ; set carry: write
        rst     0x30
        ret
