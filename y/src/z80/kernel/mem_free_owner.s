        ; Free every heap block belonging to one owner.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mem_free_owner
        .optsdcc -mz80 sdcccall(1)

        .globl  _mem_free_owner
        .globl  _mem_free
        .globl  __mem_payload_address

        .area   _CODE

        ; _mem_free_owner, sdcccall(1)
        ; inputs: hl = heap, de = owner
        ; outputs: a = freed block count, modulo 256
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; iy = heap, de = owner, b = guard, c = count.
        ; Restart at the heap head after every matching allocation.
_mem_free_owner:
        push    ix
        push    iy
        push    hl
        pop     iy
        ld      bc, #0
.owner_scan:
        ld      a, h
        or      l
        jr      z, .owner_done
        push    hl
        pop     ix
        bit     0, 4(ix)
        jr      z, .owner_next
        ld      l, 2(ix)
        ld      h, 3(ix)
        or      a
        sbc     hl, de
        jr      nz, .owner_next
        push    bc
        push    de
        call    __mem_payload_address
        push    iy
        pop     hl
        call    _mem_free
        pop     de
        pop     bc
        inc     c
        ld      b, #0
        push    iy
        pop     hl
        jr      .owner_scan
.owner_next:
        ld      l, 0(ix)
        ld      h, 1(ix)
        inc     b
        jr      nz, .owner_scan
.owner_done:
        ld      a, c
        pop     iy
        pop     ix
        ret
