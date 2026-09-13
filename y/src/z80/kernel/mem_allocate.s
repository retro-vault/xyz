        ; Allocate a block from one YOS heap arena.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mem_allocate
        .optsdcc -mz80 sdcccall(1)

        .globl  __mem_split
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
        call    __mem_split            ; splits only a usable remainder
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
