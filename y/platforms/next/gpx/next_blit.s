        ;; GPL-2.0; batched Layer 2 bitmap blitter for the 320x256x8 column
        ;; layout (byte = x * 256 + y).
        ;; One critical section for the whole bitmap and one bank select per
        ;; 64 pixels, instead of one per pixel.
        ;; _next_blit_args (14 bytes):
        ;;   0 src   first source byte (row r0, column byte of c0)
        ;;   2 step  source bytes per row (mask bitmaps: both planes)
        ;;   3 plane offset of the ink plane from the mask plane
        ;;   4 mask  non-zero for a masked bitmap
        ;;   5 bit   source bit mask of the first column
        ;;   6 cols (word)  8 rows  9 px (word)  11 py
        ;;   12 ink kind  13 paper kind  (0 skip, 1 black, 2 white, 3 xor)
        .module next_blit
        .optsdcc -mz80 sdcccall(1)
        .globl _next_blit, _next_blit_args, __next_blit_prepare
        .globl _enter_critical_section, _leave_critical_section
        .area _BSS
_next_blit_args::
        .ds 14
bank_v:  .ds 1
cols_v:  .ds 2
bit_v:   .ds 1
last_ops: .ds 2                 ; 0000 matches the initial all-NOP operations
        .area _GPX
_next_blit::
        push ix
        push iy
        call _enter_critical_section
        ld a,#0xff
        ld (bank_v),a
        ld hl,(_next_blit_args+6)
        ld (cols_v),hl
        ld a,(_next_blit_args+5)
        ld (bit_v),a
        ld hl,(_next_blit_args+12)
        ld de,(last_ops)
        or a
        sbc hl,de
        jr z,.operations_ready
        add hl,de
        ld (last_ops),hl
        call __next_blit_prepare
.operations_ready:
        ld bc,#0x123b
        ld a,#7
        out (c),a
.column:
        ld hl,(_next_blit_args+9)
        ld a,l
        rlca
        rlca
        and #3
        ld b,a
        ld a,h
        add a,a
        add a,a
        or b
        ld b,a                  ; B = framebuffer bank 0..4
        ld a,l
        and #0x3f
        ld h,a
        ld a,(_next_blit_args+11)
        ld l,a                  ; HL = address of the first row
        ld a,(bank_v)
        cp b
        jr z,.bank_ready
        ld a,b
        ld (bank_v),a
        or #0x10
        ld bc,#0x123b
        out (c),a
.bank_ready:
        ld ix,(_next_blit_args+0)
        ld a,(_next_blit_args+2)
        ld c,a
        ld b,#0
        ld a,(_next_blit_args+8)
        ld e,a
        ld a,(bit_v)
        ld d,a
        ld a,(_next_blit_args+4)
        or a
        jr nz,.mask_setup
.plain_row:
        ld a,0(ix)
        and d
        jr z,.plain_paper
__next_blit_ink_a::
        .db 0,0,0             ; patched pixel operation (see .blk_tab)
        jr .plain_next
.plain_paper:
__next_blit_paper_a::
        .db 0,0,0
.plain_next:
        inc l
        add ix,bc
        dec e
        jr nz,.plain_row
        jr .column_done
.mask_setup:
        push ix
        pop iy
        ld a,(_next_blit_args+3)
        ld c,a
        add iy,bc
        ld a,(_next_blit_args+2)
        ld c,a
.mask_rows:
        ld a,0(iy)
        and d
        jr nz,.mask_ink
        ld a,0(ix)
        and d
        jr nz,.mask_next        ; transparent
__next_blit_paper_b::
        .db 0,0,0
        jr .mask_next
.mask_ink:
__next_blit_ink_b::
        .db 0,0,0
.mask_next:
        inc l
        add ix,bc
        add iy,bc
        dec e
        jr nz,.mask_rows
.column_done:
        ld a,(bit_v)
        rrca
        ld (bit_v),a
        jr nc,.same_byte
        ld hl,(_next_blit_args+0)
        inc hl
        ld (_next_blit_args+0),hl
.same_byte:
        ld hl,(_next_blit_args+9)
        inc hl
        ld (_next_blit_args+9),hl
        ld hl,(cols_v)
        dec hl
        ld (cols_v),hl
        ld a,h
        or l
        jp nz,.column
        ld bc,#0x123b
        ld a,#2
        out (c),a
        call _leave_critical_section
        pop iy
        pop ix
        ret
