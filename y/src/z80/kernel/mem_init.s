        ; Initialize one YOS heap arena.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mem_init
        .optsdcc -mz80 sdcccall(1)

        .globl  _mem_init

        .area   _CODE

        ; _mem_init, sdcccall(1)
        ; inputs: hl = heap, de = total size
        ; outputs: none
        ; clobbers: af, de, hl; preserves ix and iy
_mem_init:
        ld      a, e
        sub     #7
        ld      e, a
        ld      a, d
        sbc     a, #0
        ld      d, a
        xor     a
        ld      (hl), a
        inc     hl
        ld      (hl), a
        inc     hl
        ld      (hl), a
        inc     hl
        ld      (hl), a
        inc     hl
        ld      (hl), a
        inc     hl
        ld      (hl), e
        inc     hl
        ld      (hl), d
        ret
