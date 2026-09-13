        ; Split a heap block without changing ownership or allocation flags.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _mem_split
        .optsdcc -mz80 sdcccall(1)
        .globl __mem_split
        .area _CODE
        ; IX=block, DE=retained prefix size; the caller holds the allocator
        ; critical section. Splits only when DE fits and the remainder can
        ; carry a header and payload (12 bytes or more): carry clear,
        ; HL=new tail payload, BC=its header. Otherwise carry set and the
        ; block is unchanged.
        ; DE unchanged; clobbers AF/BC/HL; preserves IX/IY.
__mem_split::
        ld      l, 5(ix)
        ld      h, 6(ix)
        or      a
        sbc     hl, de                 ; remainder past the retained prefix
        ret     c                      ; larger than the block
        ld      a, h
        or      a
        jr      nz, .split
        ld      a, l
        cp      #12
        ret     c                      ; too small for a header and payload
.split:
        push    de
        ld      bc, #-7
        add     hl, bc
        push    hl
        push    ix
        pop     hl
        or      a
        sbc     hl, bc                 ; header size back: IX + 7
        add     hl, de                 ; new block address; carry clear
        push    hl
        ex      de, hl
        push    ix
        pop     hl
        ld      bc, #5                 ; next, owner, status
        ldir
        pop     bc
        pop     hl                     ; new tail payload size
        ld      a, l
        ld      (de), a
        inc     de
        ld      a, h
        ld      (de), a
        inc     de                     ; new tail payload
        ex      de, hl
        pop     de
        ld      5(ix), e
        ld      6(ix), d
        ld      0(ix), c
        ld      1(ix), b
        ret                            ; carry still clear: split performed
