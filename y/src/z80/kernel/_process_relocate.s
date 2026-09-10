        ; Validate and relocate an XL payload in place.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _process_relocate
        .optsdcc -mz80 sdcccall(1)

        .globl  __process_relocate

        .area   _CODE

        ; input: HL = XL image, DE = complete XL byte length.
        ; output: DE = relocated code base, or zero for a malformed image.
        ; preserves IX and IY.
__process_relocate::
        push    ix
        push    iy
        push    hl
        pop     ix
        ld      a,d
        or      a
        jr      nz,.header
        ld      a,e
        cp      #12
        jp      c,.bad
.header:
        ld      a,0(ix)
        cp      #'X'
        jp      nz,.bad
        ld      a,1(ix)
        cp      #'L'
        jp      nz,.bad
        ld      a,2(ix)
        dec     a
        jp      nz,.bad
        ld      c,8(ix)
        ld      b,9(ix)                 ; BC = relocation count
        ld      l,c
        ld      h,b
        add     hl,hl
        jp      c,.bad
        add     hl,hl
        jp      c,.bad
        push    bc                      ; saved relocation count
        ld      bc,#12
        add     hl,bc
        jr      c,.bad_count
        push    ix
        pop     iy
        push    hl
        pop     bc
        add     iy,bc                   ; IY = code base
        ld      c,6(ix)
        ld      b,7(ix)                 ; BC = code size
        add     hl,bc
        jr      c,.bad_count
        or      a
        sbc     hl,de
        jr      nz,.bad_count

        pop     bc                      ; relocation count
        push    ix
        pop     de
        ld      hl,#12
        add     hl,de
        ex      de,hl                   ; DE = relocation table
.reloc:
        ld      a,b
        or      c
        jr      z,.ok
        ld      a,(de)
        ld      l,a
        inc     de
        ld      a,(de)
        ld      h,a                     ; HL = code-relative offset
        inc     de
        ld      a,(de)                  ; relocation width
        inc     de
        push    bc
        push    af
        ld      c,6(ix)
        ld      b,7(ix)
        push    hl
        cp      #1
        jr      z,.span_ready
        cp      #2
        jr      nz,.bad_entry
        inc     hl
        ld      a,h
        or      l
        jr      z,.bad_entry
.span_ready:
        or      a
        sbc     hl,bc
        jr      nc,.bad_entry
        pop     hl
        pop     af
        push    iy
        pop     bc
        add     hl,bc                   ; HL = patch address
        cp      #2
        jr      z,.word
        ld      a,(de)
        and     #0xfe
        jr      nz,.bad_patch
        ld      a,(de)
        rra
        ld      a,(hl)
        jr      c,.byte_high
        add     a,c
        ld      (hl),a
        jr      .next
.byte_high:
        add     a,b
        ld      (hl),a
        jr      .next
.word:
        ld      a,(de)
        or      a
        jr      nz,.bad_patch
        ld      a,(hl)
        add     a,c
        ld      (hl),a
        inc     hl
        ld      a,(hl)
        adc     a,b
        ld      (hl),a
.next:
        inc     de
        pop     bc
        dec     bc
        jr      .reloc
.bad_entry:
        pop     hl
        pop     af
        pop     bc
        jr      .bad
.bad_patch:
        pop     bc
        jr      .bad
.bad_count:
        pop     bc
.bad:
        ld      de,#0
        pop     iy
        pop     ix
        ret
.ok:
        push    iy
        pop     de
        pop     iy
        pop     ix
        ret
