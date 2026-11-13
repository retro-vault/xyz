        ;; GPL-2.0; pixel column fill for the libgpx Next adapter.
        .module next_fill
        .optsdcc -mz80 sdcccall(1)
        .globl _next_fill_column, _next_fill_img, __next_fill_rows
        .globl fill_rows, fi_x, fi_cnt, fi_step, fi_mask, fi_flags, fi_y, fi_h
        .globl _enter_critical_section, _leave_critical_section
        .area _BSS
        ;; Parameters of _next_fill_img (set by the fill front end).
fi_x::     .ds 2                ; first pixel column
fi_cnt::   .ds 2                ; number of columns
fi_h::     .ds 2                ; rows (1..256)
fi_step::  .ds 1                ; column step
fi_mask::  .ds 1                ; pattern bit of the pixel
fi_flags:: .ds 1                ; bit0: black ink, bit1: repeating row pair
fi_y::     .ds 1                ; first row
fi_n:      .ds 1
fi_bank:   .ds 1
        .area _GPX
;; HL=x (pixel 0..319), DE=y; stack: height(2), copied pattern rows(2),
;; phase(1), flags(1).
;; Flags: ink bit0 (1 black), XOR bit1, OR bit2. Each pixel is one byte.
;; Map only one column at a time, restoring the kernel between columns.
_next_fill_column::
        push ix
        ld ix,#0
        add ix,sp
        push iy
        ld a,h
        add a,a
        add a,a
        ld b,a
        ld a,l
        rlca
        rlca
        and #3
        or b
        ld d,a                  ; bank offset 0..4
        ld a,l
        and #0x3f
        ld h,a
        ld l,e                  ; column byte offset is its Y coordinate
        call _enter_critical_section
        ld bc,#0x123b
        ld a,#7
        out (c),a
        ld a,d
        or #0x10
        out (c),a
        ld e,4(ix)              ; 0 means 256 rows
        ld c,6(ix)
        ld b,7(ix)              ; BC = copied pattern rows
        ld d,8(ix)              ; one pattern bit for this pixel column
        ld a,9(ix)
        and #1
        neg
        cpl
        ld iyl,a                ; ink byte: black 0, white FF
        ;; COPY uses the image path. OR/XOR skip every zero pattern bit;
        ;; read through BC instead of an indexed address for every row.
        bit 1,9(ix)
        jr nz,.xor_rows
.or_rows:
        ld a,(bc)
        inc bc
        and d
        jr z,.or_skip
        ld a,iyl
        ld (hl),a
.or_skip:
        inc l                   ; clipping keeps all rows in this column
        dec e
        jr nz,.or_rows
        jr .column_done
.xor_rows:
        ld a,(bc)
        inc bc
        and d
        jr z,.xor_skip
        ld a,(hl)
        cpl
        ld (hl),a
.xor_skip:
        inc l
        dec e
        jr nz,.xor_rows
.column_done:
        ld bc,#0x123b
        ld a,#2
        out (c),a
        call _leave_critical_section
        pop iy
        pop ix
        pop hl
        pop bc
        pop bc
        pop bc
        jp (hl)

;; Paint pixel columns of a plain COPY fill from one column image.
;; fill_rows holds the pattern byte of every row on entry; it is turned
;; into the column image in place (pattern one -> ink, zero -> paper), and
;; fi_cnt columns, fi_step apart from fi_x, receive the image. Repeating row
;; pairs need only two converted pixels and use word writes; other patterns
;; copy their complete row image. All
;; arguments are in the fi_* block. Columns are painted in chunks so that
;; interrupts are blocked for at most ~16 columns at a time.
_next_fill_img::
        ld e,#2
        ld a,(fi_flags)
        bit 1,a
        jr nz,.convert_rows
        ld a,(fi_h)
        ld e,a
.convert_rows:
        call __next_fill_rows
.fi_chunk:
        ld hl,(fi_cnt)
        ld a,h
        or l
        ret z
        ld a,h
        or a
        jr nz,.fi_n16
        ld a,l
        cp #16
        jr c,.fi_nset
.fi_n16:
        ld a,#16
.fi_nset:
        ld (fi_n),a
        ld e,a
        ld d,#0
        or a
        sbc hl,de
        ld (fi_cnt),hl
        call _enter_critical_section
        ld bc,#0x123b
        ld a,#7                 ; Layer 2 read/write overlay at 0000
        out (c),a
        ld a,#0xff
        ld (fi_bank),a
.fi_col:
        ld hl,(fi_x)
        ld a,h
        add a,a
        add a,a
        ld b,a
        ld a,l
        rlca
        rlca
        and #3
        or b
        ld e,a                  ; framebuffer bank offset 0..4
        ld a,l
        and #0x3f
        ld h,a
        ld a,(fi_y)
        ld l,a                  ; HL = column address
        ld a,(fi_bank)
        cp e
        jr z,.fi_bank_ok
        ld a,e
        ld (fi_bank),a
        or #0x10
        ld bc,#0x123b
        out (c),a
.fi_bank_ok:
        ld d,h
        ld e,l
        ld hl,#fill_rows
        ld bc,(fi_h)
        ld a,(fi_flags)
        bit 1,a
        jr z,.fi_copy_general
        call .pair_fill
        jr .fi_copied
.fi_copy_general:
        call .copy16
.fi_copied:
        ld hl,(fi_x)
        ld a,(fi_step)
        ld e,a
        ld d,#0
        add hl,de
        ld (fi_x),hl
        ld hl,#fi_n
        dec (hl)
        jr nz,.fi_col
        ld bc,#0x123b
        ld a,#2                 ; back to YOS before releasing interrupts
        out (c),a
        call _leave_critical_section
        jp .fi_chunk

;; Complete bytes, sixteen at a time: LDI costs 16 rather than LDIR's
;; repeated 21 T-states. The tail retains exact clipped column bounds.
.copy16:
        ld a,b
        or a
        jr nz,.copy_block
        ld a,c
        cp #16
        jr c,.copy_tail
.copy_block:
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        jp po,.copy_done
        ld a,c
        cp #16
        jr nc,.copy_block
.copy_tail:
        ld a,b
        or c
        ret z
        ldir
.copy_done:
        ret

;; One-/two-row COPY patterns repeat the same word down each pixel column.
;; Write sixteen pixels with eight PUSH instructions instead of sixteen LDI.
;; Interrupts remain locked and SP is restored before any CALL/RET or banking.
;; DE=first destination pixel, BC=height (1..256). Clobbers AF/BC/DE/HL.
.pair_fill:
        ld (.pair_restore+1),sp
        ld h,d
        ld l,e
        add hl,bc               ; one past the last pixel, at most 4000h
        bit 0,c
        jr z,.pair_even
        dec hl                  ; odd final row has the first row's color
        ld a,(fill_rows)
        ld (hl),a
.pair_even:
        srl b
        rr c                    ; number of pairs, up to 128
        ld a,c
        and #7
        ld e,a                  ; tail pairs
        ld a,c
        srl a
        srl a
        srl a
        ld b,a                  ; groups of eight pairs
        ld c,e
        ld de,(fill_rows)
        ld sp,hl
        or a
        jr z,.pair_tail
.pair_group:
        push de
        push de
        push de
        push de
        push de
        push de
        push de
        push de
        djnz .pair_group
.pair_tail:
        ld b,c
        ld a,b
        or a
        jr z,.pair_restore
.pair_one:
        push de
        djnz .pair_one
.pair_restore:
        ld sp,#0                ; patched while the real stack is mapped
        ret
