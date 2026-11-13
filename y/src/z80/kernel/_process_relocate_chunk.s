        ; Apply one bounded chunk of XL relocation records.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _process_relocate_chunk
        .optsdcc -mz80 sdcccall(1)
        .globl  __process_relocate_chunk
        .equ    IMAGE_CODE, 68
        .area   _CODE

        ; inputs: IX = loader frame, HL = records, B = record count (1..32).
        ; Frame bytes 12..13 hold the exclusive code end and 68..69 the base.
        ; output: A = zero / 4 malformed record; preserves IX and IY.
__process_relocate_chunk::
        push    iy
        push    hl
        pop     iy
.record:
        ld      l, 0(iy)
        ld      h, 1(iy)
        ld      a, 2(iy)
        cp      #1
        jr      z, .byte
        cp      #2
        jr      nz, .bad
.word:
        ld      a, 3(iy)
        or      a
        jr      nz, .bad
        push    bc
        ld      c, IMAGE_CODE(ix)
        ld      b, IMAGE_CODE+1(ix)
        add     hl, bc
        jr      c, .bad_pop
        ld      e, 12(ix)
        ld      d, 13(ix)
        push    hl
        inc     hl
        ld      a, h
        or      l
        jr      z, .bad_span
        or      a
        sbc     hl, de
        pop     hl
        jr      nc, .bad_pop
        ld      a, (hl)
        add     a, c
        ld      (hl), a
        inc     hl
        ld      a, (hl)
        adc     a, b
        ld      (hl), a
        jr      .next
.bad_span:
        pop     hl
        jr      .bad_pop
.byte:
        ld      a, 3(iy)
        cp      #2
        jr      nc, .bad
        push    bc
        ld      c, IMAGE_CODE(ix)
        ld      b, IMAGE_CODE+1(ix)
        add     hl, bc
        jr      c, .bad_pop
        ld      e, 12(ix)
        ld      d, 13(ix)
        push    hl
        or      a
        sbc     hl, de
        pop     hl
        jr      nc, .bad_pop
        ld      a, 3(iy)
        or      a
        ld      a, c
        jr      z, .byte_add
        ld      a, b
.byte_add:
        add     a, (hl)
        ld      (hl), a
.next:
        pop     bc
        ld      de, #4
        add     iy, de
        djnz    .record
        xor     a
        pop     iy
        ret
.bad_pop:
        pop     bc
.bad:
        ld      a, #4
        pop     iy
        ret
