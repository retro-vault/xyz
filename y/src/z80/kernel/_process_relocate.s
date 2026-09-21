        ; Validate an XL payload and relocate its code in its existing buffer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _process_relocate
        .optsdcc -mz80 sdcccall(1)

        .globl  __process_relocate

        .area   _CODE

        ; input: HL = XL image, DE = complete XL byte length,
        ;        BC = bytes reserved before the code (service exports).
        ; output: HL = future resident prefix, DE = relocated code base,
        ;         BC = XL code size, A = zero / 4 malformed image.
        ; XL version 2 stores the relocation table after the code, so the
        ; code stays at header + 12 and the consumed table ends the buffer.
        ; No allocation is performed. Metadata stays readable until the
        ; loader binds exports and calls __image_retain to split off and
        ; free the relocation table and the metadata prefix.
        ; preserves IX and IY.
__process_relocate::
        push    ix
        push    iy
        push    bc                      ; resident prefix size
        push    hl
        pop     ix                      ; source XL header
        ; A short image fails the exact length check below, so the header
        ; bytes may be read without a separate minimum-size test.
        ld      a,0(ix)
        cp      #'X'
        jp      nz,.bad_saved
        ld      a,1(ix)
        cp      #'L'
        jp      nz,.bad_saved
        ld      a,2(ix)
        cp      #2
        jp      nz,.bad_saved
        ld      c,8(ix)
        ld      b,9(ix)                 ; BC = relocation count
        ld      l,c
        ld      h,b
        add     hl,hl
        jp      c,.bad_saved
        add     hl,hl
        jp      c,.bad_saved            ; HL = relocation table bytes
        ld      c,6(ix)
        ld      b,7(ix)                 ; BC = code size
        add     hl,bc
        jr      c,.bad_saved
        ld      bc,#12
        add     hl,bc
        jr      c,.bad_saved            ; header + code + table
        or      a
        sbc     hl,de
        jr      nz,.bad_saved           ; must equal the complete XL length
        push    ix
        pop     iy
        add     iy,bc                   ; IY = code base, header + 12
        ld      c,6(ix)
        ld      b,7(ix)
        push    iy
        pop     hl
        add     hl,bc
        ex      de,hl                   ; DE = source relocation table
        pop     bc                      ; resident prefix size
        push    iy
        pop     hl
        or      a
        sbc     hl,bc                   ; compact exports immediately before code
        push    hl                      ; future resident allocation
        ld      c,8(ix)
        ld      b,9(ix)                 ; relocation count
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
        add     hl,bc                   ; destination patch address
        cp      #2
        jr      z,.word
        ld      a,(de)
        srl     a                       ; carry: patch with the high base byte
        jr      nz,.bad_patch
        ld      a,c
        jr      nc,.byte_add
        ld      a,b
.byte_add:
        add     a,(hl)
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
.bad_patch:
        pop     bc
.bad_saved:
        pop     hl                      ; prefix size or future resident
        ld      hl,#0
        ld      de,#0
        ld      a,#4
        pop     iy
        pop     ix
        ret
.ok:
        pop     hl                      ; resident allocation
        push    iy
        pop     de
        ld      c,6(ix)
        ld      b,7(ix)                 ; code size for the loader frame
        xor     a
        pop     iy
        pop     ix
        ret
