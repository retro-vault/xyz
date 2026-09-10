        ; Compute the IEEE CRC-32 used by XPRG images.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _crc32
        .optsdcc -mz80 sdcccall(1)

        .globl  __crc32

        .area   _CODE

        ; input: HL = bytes, BC = length.
        ; output: DE:HL = IEEE CRC-32; preserves IX and IY.
__crc32::
        push    ix
        push    hl
        pop     ix
        ld      de,#0xffff
        ld      hl,#0xffff
.byte:
        ld      a,b
        or      c
        jr      z,.finish
        ld      a,l
        xor     0(ix)
        ld      l,a
        inc     ix
        push    bc
        ld      b,#8
.bit:
        srl     d
        rr      e
        rr      h
        rr      l
        jr      nc,.next_bit
        ld      a,d
        xor     #0xed
        ld      d,a
        ld      a,e
        xor     #0xb8
        ld      e,a
        ld      a,h
        xor     #0x83
        ld      h,a
        ld      a,l
        xor     #0x20
        ld      l,a
.next_bit:
        djnz    .bit
        pop     bc
        dec     bc
        jr      .byte
.finish:
        ld      a,d
        cpl
        ld      d,a
        ld      a,e
        cpl
        ld      e,a
        ld      a,h
        cpl
        ld      h,a
        ld      a,l
        cpl
        ld      l,a
        pop     ix
        ret
