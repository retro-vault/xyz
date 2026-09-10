        ; Free and coalesce a block in one YOS heap arena.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mem_free
        .optsdcc -mz80 sdcccall(1)

        .globl  _mem_free
        .globl  __mem_payload_address

        .area   _CODE

        ; _mem_free, sdcccall(1)
        ; inputs: hl = heap, de = payload
        ; outputs: de = merged payload or zero if absent
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; ix holds the predecessor, de the sought block while scanning.
_mem_free:
        push    ix
        ld      ix, #0
        push    hl
        ex      de, hl
        ld      bc, #-7
        add     hl, bc
        ex      de, hl
        pop     hl
        ld      b, #0
.free_scan:
        ld      a, h
        or      l
        jr      z, .free_failed
        or      a
        sbc     hl, de
        add     hl, de
        jr      z, .free_found
        push    hl
        pop     ix
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        inc     b
        jr      nz, .free_scan
.free_failed:
        ld      de, #0
        pop     ix
        ret
.free_found:
        push    hl
        inc     hl
        inc     hl
        xor     a
        ld      (hl), a
        inc     hl
        ld      (hl), a
        inc     hl
        ld      (hl), a
        push    ix
        pop     hl
        ld      a, h
        or      l
        jr      z, .free_current
        bit     0, 4(ix)
        jr      nz, .free_current
        call    .merge_next
        pop     hl
        jr      .free_next
.free_current:
        pop     ix
.free_next:
        ld      l, 0(ix)
        ld      h, 1(ix)
        ld      a, h
        or      l
        jr      z, .free_done
        ld      bc, #4
        add     hl, bc
        bit     0, (hl)
        call    z, .merge_next
.free_done:
        call    __mem_payload_address
        pop     ix
        ret

        ; ix = block with a non-null next block; ix is unchanged.
        ; Preserve the reference's size assignment before reading next.
.merge_next:
        ld      l, 0(ix)
        ld      h, 1(ix)
        push    hl
        ld      de, #5
        add     hl, de
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      l, 5(ix)
        ld      h, 6(ix)
        add     hl, de
        ld      de, #7
        add     hl, de
        ld      5(ix), l
        ld      6(ix), h
        pop     hl
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      0(ix), e
        ld      1(ix), d
        ret
