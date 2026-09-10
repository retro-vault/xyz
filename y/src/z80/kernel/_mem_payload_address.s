        ; Shared conversion from a heap header to its payload.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _mem_payload_address
        .optsdcc -mz80 sdcccall(1)

        .globl  __mem_payload_address

        .area   _CODE

        ; __mem_payload_address, internal register helper
        ; inputs: ix = block header
        ; outputs: de = payload address
        ; clobbers: de, hl; preserves ix and iy
__mem_payload_address::
        push    ix
        pop     hl
        ld      de, #7
        add     hl, de
        ex      de, hl
        ret
