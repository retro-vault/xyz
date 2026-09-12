        ; Allocate a block from one YOS heap arena.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mem_allocate
        .optsdcc -mz80 sdcccall(1)

        .globl  _mem_allocate
        .globl  __mem_payload_address

        .area   _CODE

        ; _mem_allocate, sdcccall(1), callee removes owner argument
        ; inputs: hl = heap, de = requested size, owner at 2(sp)
        ; outputs: de = payload or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; While scanning, b counts transitions modulo 256.
_mem_allocate:
        push    ix
        ld      b, #0
.allocate_scan:
        ld      a, h
        or      l
        jp      z, .allocate_failed
        push    hl
        pop     ix
        bit     0, 4(ix)
        jr      nz, .allocate_next
        ld      l, 5(ix)
        ld      h, 6(ix)
        or      a
        sbc     hl, de
        jr      c, .allocate_next
        ld      a, h
        or      a
        jr      nz, .allocate_split
        ld      a, l
        cp      #12
        jr      c, .allocate_claim
.allocate_split:
        ; Keep the strict size difference > 7 + 4 condition.
        push    de
        ld      bc, #-7
        add     hl, bc
        push    hl
        push    ix
        pop     hl
        ld      bc, #7
        add     hl, bc
        add     hl, de
        push    hl                     ; new block address
        ex      de, hl
        push    ix
        pop     hl
        ld      bc, #5                 ; next, owner, status
        ldir
        pop     bc
        pop     hl                     ; new free payload size
        ld      a, l
        ld      (de), a
        inc     de
        ld      a, h
        ld      (de), a
        pop     de
        ld      5(ix), e
        ld      6(ix), d
        ld      0(ix), c
        ld      1(ix), b
.allocate_claim:
        ld      hl, #4
        add     hl, sp
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      2(ix), e
        ld      3(ix), d
        ld      4(ix), #1
        call    __mem_payload_address
        jr      .allocate_return
.allocate_next:
        ld      l, 0(ix)
        ld      h, 1(ix)
        inc     b
        jp      nz, .allocate_scan
.allocate_failed:
        ld      de, #0
.allocate_return:
        pop     ix
        pop     hl
        pop     bc
        jp      (hl)
