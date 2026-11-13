        ; Compute the IEEE CRC-32 used by XPRG images.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _crc32
        .optsdcc -mz80 sdcccall(1)

        .globl  __crc32, __crc32_begin, __crc32_update, __crc32_end
        .area _BSS
.state: .ds 4

        .area   _CODE

        ; input: HL = bytes, BC = length.
        ; output: DE:HL = IEEE CRC-32; preserves IX and IY.
__crc32::
        call __crc32_begin
        call __crc32_update
        jp __crc32_end
__crc32_begin::
        ld a,#255
        ld (.state),a
        ld (.state+1),a
        ld (.state+2),a
        ld (.state+3),a
        ret
        ;; Incremental CRC over HL/BC bytes; preserves IX/IY.
__crc32_update::
        push ix
        push hl
        pop ix
        ld hl,(.state)
        ld de,(.state+2)
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
        ld (.state),hl
        ld (.state+2),de
        pop ix
        ret
__crc32_end::
        ld hl,(.state)
        ld de,(.state+2)
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
        ret
