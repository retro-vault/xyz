        ;; GPL-2.0; libgpx ABI front ends for the Next Layer 2 320x256 driver,
        ;; all in assembly. sdcccall(1): HL, DE carry the first two arguments,
        ;; the rest are on the stack and are removed by the callee. IX and IY
        ;; are preserved. Every entry holds one nestable critical section, so
        ;; the static scratch below cannot be shared with another thread.
        ;; Pixels use _next_sample, bitmaps and solid lines _next_blit, fills
        ;; _next_fill_column.
        .module next_gfx
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_draw_pixel, _gpx_draw_line, _gpx_draw_bmp, _gpx_draw_bmp_clip
        .globl _gpx_measure_text, _gpx_draw_text, _gpx_fill_rectangle
        .globl _gpx_show_sprite_soft, _gpx_hide_sprite_soft, _next_raw
        .globl _next_sample, _next_blit, _next_blit_args, _next_fill_column
        .globl _next_fill_img, fill_rows, fi_x, fi_cnt, fi_step, fi_mask, fi_flags, fi_y, fi_h
        .globl _enter_critical_section, _leave_critical_section
        .area _BSS
vis_x0: .ds 2
vis_y0: .ds 2
vis_x1: .ds 2
vis_y1: .ds 2
ax_pos: .ds 2
ax_len: .ds 2
ax_lo:  .ds 2
ax_hi:  .ds 2
ax_off: .ds 2
ax_cnt: .ds 2
g_x:    .ds 2
g_y:    .ds 2
g_b:    .ds 2
g_clip: .ds 2
g_c:    .ds 1
g_m:    .ds 1
g_opaque: .ds 1
g_w:    .ds 1
g_h:    .ds 1
g_stride: .ds 1
g_enc:  .ds 1
g_c0:   .ds 1
g_r0:   .ds 1
fr_c:   .ds 1
fr_m:   .ds 1
fr_len: .ds 1
fr_pat: .ds 2
fr_x0:  .ds 2
fr_y0:  .ds 2
fr_x1:  .ds 2
fr_y1:  .ds 2
fr_lo:  .ds 2
fr_hi:  .ds 2
fr_top: .ds 2
fr_bot: .ds 2
fr_h:   .ds 2
fr_flags: .ds 1
txt_rect: .ds 8
fc_x:   .ds 2
fc_p:   .ds 1
fc_jlo: .ds 2
fc_jhi: .ds 2
fill_rows:: .ds 256
        .area _CODE

;; ===================================================================== helpers
;; Signed compare: carry set if HL < DE. Preserves HL, DE.
gx_cmp_s::
        push hl
        ld a,h
        xor d
        jp m,.signs_1
        or a
        sbc hl,de
        pop hl
        ret
.signs_1:
        ld a,h
        rlca
        pop hl
        ret

;; HL = value, BC = &variable (word): keep the larger / smaller.
gx_upd_max::
        ld a,(bc)
        ld e,a
        inc bc
        ld a,(bc)
        ld d,a
        dec bc
        call gx_cmp_s
        ret c
        jr gx_upd_store
gx_upd_min::
        ld a,(bc)
        ld e,a
        inc bc
        ld a,(bc)
        ld d,a
        dec bc
        call gx_cmp_s
        ret nc
gx_upd_store::
        ld a,l
        ld (bc),a
        inc bc
        ld a,h
        ld (bc),a
        ret

;; HL = a, DE = b -> HL = min, DE = max (signed).
gx_minmax::
        call gx_cmp_s
        ret c
        ex de,hl
        ret

;; Screen intersected with the clip rectangle at (g_clip) -> vis_*.
gx_make_vis::
        ld hl,#0
        ld (vis_x0),hl
        ld (vis_y0),hl
        ld hl,#319
        ld (vis_x1),hl
        ld hl,#255
        ld (vis_y1),hl
        ld hl,(g_clip)
        ld a,h
        or l
        ret z
        push ix
        push hl
        pop ix
        ld l,0(ix)
        ld h,1(ix)
        ld bc,#vis_x0
        call gx_upd_max
        ld l,2(ix)
        ld h,3(ix)
        ld bc,#vis_y0
        call gx_upd_max
        ld l,4(ix)
        ld h,5(ix)
        ld bc,#vis_x1
        call gx_upd_min
        ld l,6(ix)
        ld h,7(ix)
        ld bc,#vis_y1
        call gx_upd_min
        pop ix
        ret

;; Visible span of [ax_pos, ax_pos+ax_len) inside [ax_lo, ax_hi]:
;; ax_off = first gx_visible offset, ax_cnt = count. Carry set = nothing gx_visible.
gx_axis::
        ld hl,(ax_lo)
        ld de,(ax_pos)
        or a
        sbc hl,de
        bit 7,h
        jr z,.off_ok_7
        ld hl,#0
.off_ok_7:
        ld (ax_off),hl
        ld hl,(ax_hi)
        ld de,(ax_pos)
        or a
        sbc hl,de
        ld de,(ax_len)
        dec de
        call gx_cmp_s
        jr c,.last_ready_7
        ex de,hl
.last_ready_7:
        ld de,(ax_off)
        call gx_cmp_s
        ret c
        or a
        sbc hl,de
        inc hl
        ld (ax_cnt),hl
        or a
        ret

;; HL = value, BC = &low word (high word at +4). Carry set = outside.
;; Preserves HL.
gx_rng::
        push hl
        ld a,(bc)
        ld e,a
        inc bc
        ld a,(bc)
        ld d,a
        call gx_cmp_s
        jr c,.bad_8
        inc bc
        inc bc
        inc bc
        ld a,(bc)
        ld e,a
        inc bc
        ld a,(bc)
        ld d,a
        ex de,hl
        call gx_cmp_s
        ex de,hl
        jr c,.bad_8
        pop hl
        or a
        ret
.bad_8:
        pop hl
        scf
        ret

;; HL = x, DE = y against vis_*. Carry set = not gx_visible. Preserves HL, DE.
gx_visible::
        push de
        ld bc,#vis_x0
        call gx_rng
        jr c,.vis_no_9
        ex (sp),hl
        ld bc,#vis_y0
        call gx_rng
        ex (sp),hl
        jr c,.vis_no_9
        pop de
        or a
        ret
.vis_no_9:
        pop de
        scf
        ret

;; These helpers run before the framebuffer overlay is enabled. Keep them
;; in the otherwise unused header gap to free resident raster space.
        .area _HEADER_DATA
;; A * C -> HL (C small).
gx_mul_a_c::
        ld hl,#0
        or a
        ret z
        ld b,a
        ld e,c
        ld d,#0
.l_10:
        add hl,de
        djnz .l_10
        ret

;; HL = x, DE = y, C = colour, B = mode: gx_plot with the vis_* clip.
gx_plot::
        push bc                 ; gx_visible uses BC as a pointer
        call gx_visible
        pop bc
        ret c
        ld a,c
        and #1
        ld c,a
        ld a,b
        cp #1
        ld a,c
        jr nz,.flags_11
        or #2
.flags_11:
        pop bc                  ; return address goes on top of the byte argument;
        push af                 ; HL (x) and DE (y) must reach _next_sample intact
        inc sp
        push bc
        jp _next_sample

;; ink kind from g_c/g_m in A: 3 xor, 1 black, 2 white.
gx_ink_kind::
        ld a,(g_m)
        cp #1
        ld a,#3
        ret z
        ld a,(g_c)
        and #1
        ld a,#2
        ret z
        ld a,#1
        ret

;; A = gx_glyph for character A of the font at IY -> HL bitmap or 0.
gx_glyph::
        ld d,a
        ld a,1(iy)
        ld e,a
        ld a,d
        cp e
        jr c,.none_13
        ld a,2(iy)
        cp d
        jr c,.none_13
        ld a,d
        sub e
        ld l,a
        ld h,#0
        add hl,hl
        push iy
        pop de
        add hl,de
        ld de,#8
        add hl,de
        ld a,0(iy)
        and #2
        ld e,(hl)
        inc hl
        ld d,(hl)
        jr z,.le_13
        ld a,e
        ld e,d
        ld d,a
.le_13:
        ld a,d
        or e
        jr z,.none_13
        push iy
        pop hl
        add hl,de
        inc hl
        ld a,(hl)               ; zero-width glyphs use font.empty_width
        dec hl
        or a
        ret nz
.none_13:
        ld hl,#0
        ret

        .area _CODE
;; ================================================================== bitmaps
;; Draw bitmap g_b at g_x,g_y with g_c, g_m, g_clip, g_opaque.
gx_bitmap_core::
        ld hl,(g_b)
        ld a,h
        or l
        ret z
        ld a,(hl)
        ld c,a
        and #0x0f
        inc a
        ld (g_stride),a
        ld a,c
        rrca
        rrca
        rrca
        rrca
        and #0x0f
        cp #2
        ret nc
        ld (g_enc),a
        inc hl
        ld a,(hl)
        ld (g_w),a
        inc hl
        ld a,(hl)
        ld (g_h),a
        call gx_make_vis
        ld hl,(g_x)
        ld (ax_pos),hl
        ld a,(g_w)
        ld l,a
        ld h,#0
        ld (ax_len),hl
        ld hl,(vis_x0)
        ld (ax_lo),hl
        ld hl,(vis_x1)
        ld (ax_hi),hl
        call gx_axis
        ret c
        ld hl,(ax_off)
        ld a,l
        ld (g_c0),a
        ld hl,(ax_cnt)
        ld (_next_blit_args+6),hl
        ld hl,(ax_off)
        ld de,(g_x)
        add hl,de
        ld (_next_blit_args+9),hl
        ld hl,(g_y)
        ld (ax_pos),hl
        ld a,(g_h)
        ld l,a
        ld h,#0
        ld (ax_len),hl
        ld hl,(vis_y0)
        ld (ax_lo),hl
        ld hl,(vis_y1)
        ld (ax_hi),hl
        call gx_axis
        ret c
        ld a,(ax_off)
        ld (g_r0),a
        ld a,(ax_cnt)
        ld (_next_blit_args+8),a
        ld hl,(ax_off)
        ld de,(g_y)
        add hl,de
        ld a,l
        ld (_next_blit_args+11),a
        ld a,(g_stride)
        ld c,a
        ld a,(g_enc)
        or a
        jr z,.plain_14
        ld a,c
        add a,a
        ld c,a                  ; masked: AND and OR planes per row
        ld a,(g_stride)
        jr .plane_14
.plain_14:
        xor a
.plane_14:
        ld (_next_blit_args+3),a
        ld a,c
        ld (_next_blit_args+2),a
        ld a,(g_enc)
        ld (_next_blit_args+4),a
        ld a,(g_r0)
        call gx_mul_a_c
        ld a,(g_c0)
        rrca
        rrca
        rrca
        and #0x1f
        ld e,a
        ld d,#0
        add hl,de
        ld de,(g_b)
        add hl,de
        ld de,#5
        add hl,de
        ld (_next_blit_args+0),hl
        ld a,(g_c0)
        and #7
        ld b,a
        ld a,#0x80
        inc b
        dec b
        jr z,.bit_done_14
.bit_14:
        rrca
        djnz .bit_14
.bit_done_14:
        ld (_next_blit_args+5),a
        call gx_ink_kind
        ld (_next_blit_args+12),a
        ld a,(g_c)
        and #1
        ld a,#1
        jr z,.paper_kind_14
        ld a,#2
.paper_kind_14:
        ld b,a
        ld a,(g_enc)
        or a
        jr nz,.paper_set_14
        ld a,(g_opaque)
        or a
        jr nz,.paper_set_14
        ld b,#0
.paper_set_14:
        ld a,b
        ld (_next_blit_args+13),a
        jp _next_blit

;; Prologue / epilogue pieces use: push ix, push iy, ld ix,0, add ix,sp, then
;; stack parameters at 6(ix) (return address at 4(ix)).

;; gpx_draw_bmp(g=HL, x=DE; y, bmp, clip)
_gpx_draw_bmp::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        call _enter_critical_section
        ld (g_x),de
        ld l,6(ix)
        ld h,7(ix)
        ld (g_y),hl
        ld l,8(ix)
        ld h,9(ix)
        ld (g_b),hl
        ld l,10(ix)
        ld h,11(ix)
        ld (g_clip),hl
        ld a,#1
        ld (g_c),a
        xor a
        ld (g_m),a
        ld (g_opaque),a
        call gx_bitmap_core
        call _leave_critical_section
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#6
        add hl,sp
        ld sp,hl
        push bc
        ret

;; gpx_draw_bmp_clip(g=HL, x=DE; y, bmp, c, m, clip)
_gpx_draw_bmp_clip::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        call _enter_critical_section
        ld (g_x),de
        ld l,6(ix)
        ld h,7(ix)
        ld (g_y),hl
        ld l,8(ix)
        ld h,9(ix)
        ld (g_b),hl
        ld a,10(ix)
        ld (g_c),a
        ld a,11(ix)
        ld (g_m),a
        ld l,12(ix)
        ld h,13(ix)
        ld (g_clip),hl
        xor a
        ld (g_opaque),a
        call gx_bitmap_core
        call _leave_critical_section
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#8
        add hl,sp
        ld sp,hl
        push bc
        ret

;; ===================================================================== text
;; gpx_measure_text(text=HL, font=DE) -> DE
_gpx_measure_text::
        push iy
        ld a,h
        or l
        jr z,.zero_17
        ld a,d
        or e
        jr z,.zero_17
        push de
        pop iy
        ld bc,#0
.next_17:
        ld a,(hl)
        or a
        jr z,.done_17
        inc hl
        push hl
        push bc
        call gx_glyph
        ld a,h
        or l
        jr z,.empty_17
        inc hl
        ld a,(hl)
        jr .have_17
.empty_17:
        ld a,3(iy)
.have_17:
        pop bc
        add a,c
        ld c,a
        jr nc,.nocarry_17
        inc b
.nocarry_17:
        pop hl
        ld a,(hl)
        or a
        jr z,.done_17
        ld a,6(iy)
        add a,c
        ld c,a
        jr nc,.next_17
        inc b
        jr .next_17
.done_17:
        ld d,b
        ld e,c
        pop iy
        ret
.zero_17:
        ld de,#0
        pop iy
        ret

;; gpx_draw_text(g=HL, x=DE; y, text, font, c, m, clip)
;; Locals: -2 x, -4 text, -6 glyph, -7 width, -8 opaque,
;; -9 single-pass COPY, -11 right clip.
_gpx_draw_text::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        ld a,h
        or l
        jp z,.out_18
        ld a,8(ix)
        or 9(ix)
        jp z,.out_18
        ld a,10(ix)
        or 11(ix)
        jp z,.out_18
        push hl
        pop iy
        ld a,5(iy)              ; text background: 0 = opaque
        or 13(ix)               ; XOR (mode 1) always preserves background
        and #1
        xor #1
.not_opaque_18:
        ld hl,#-12
        add hl,sp
        ld sp,hl
        ld -8(ix),a
        call _enter_critical_section
        ld -2(ix),e
        ld -1(ix),d
        ld l,8(ix)
        ld h,9(ix)
        ld -4(ix),l
        ld -3(ix),h
        ld l,14(ix)
        ld h,15(ix)
        ld (g_clip),hl
        call gx_make_vis
        ld hl,(vis_x1)
        ld -11(ix),l
        ld -10(ix),h
.char_18:
        ;; No glyph can re-enter the clip after its left edge passes it.
        ld l,-11(ix)
        ld h,-10(ix)
        ld e,-2(ix)
        ld d,-1(ix)
        call gx_cmp_s
        jp c,.done_18
        ld l,-4(ix)
        ld h,-3(ix)
        ld a,(hl)
        or a
        jp z,.done_18
        inc hl
        ld -4(ix),l
        ld -3(ix),h
        ld e,10(ix)
        ld d,11(ix)
        push de
        pop iy
        call gx_glyph
        ld -6(ix),l
        ld -5(ix),h
        ld a,h
        or l
        jr z,.empty_18
        inc hl
        ld a,(hl)
        jr .width_18
.empty_18:
        ld a,3(iy)
.width_18:
        ld -7(ix),a
        ld -9(ix),#0
        ld a,-8(ix)
        or a
        jp z,.glyph_18
        ;; Plain COPY glyphs paint ink and paper in the same byte pass.
        ;; Only their advance gap needs a separate background fill.
        ld a,13(ix)
        or a
        jr nz,.legacy_cell_18
        ld l,-6(ix)
        ld h,-5(ix)
        ld a,h
        or l
        jr z,.legacy_cell_18
        ld a,(hl)
        and #0xf0
        jr nz,.legacy_cell_18
        inc hl
        inc hl
        ld a,(hl)
        cp 5(iy)
        jr nz,.legacy_cell_18
        inc -9(ix)
        ld a,6(iy)
        or a
        jp z,.glyph_18
        ld l,-4(ix)
        ld h,-3(ix)
        ld a,(hl)
        or a
        jp z,.glyph_18
.legacy_cell_18:
        ;; opaque cell: fill (x, y, x+w-1+advance?, y+height-1) with c^1
        ld l,-2(ix)
        ld h,-1(ix)
        ld a,-9(ix)
        or a
        jr z,.cell_left_18
        ld a,-7(ix)
        ld e,a
        ld d,#0
        add hl,de
.cell_left_18:
        ld (txt_rect),hl
        ld l,6(ix)
        ld h,7(ix)
        ld (txt_rect+2),hl
        ld a,-7(ix)
        ld e,a
        ld d,#0
        ld l,-2(ix)
        ld h,-1(ix)
        add hl,de
        dec hl
        push hl
        ld l,-4(ix)
        ld h,-3(ix)
        ld a,(hl)
        pop hl
        or a
        jr z,.last_cell_18
        ld a,6(iy)
        ld e,a
        ld d,#0
        add hl,de
.last_cell_18:
        ld (txt_rect+4),hl
        ld a,5(iy)
        ld e,a
        ld d,#0
        ld l,6(ix)
        ld h,7(ix)
        add hl,de
        dec hl
        ld (txt_rect+6),hl
        ld a,12(ix)
        xor #1
        ld (fr_c),a
        xor a
        ld (fr_m),a
        ld hl,#gx_solid_pattern
        ld (fr_pat),hl
        ld a,#1
        ld (fr_len),a
        ld l,14(ix)
        ld h,15(ix)
        ld (g_clip),hl
        ld hl,#txt_rect
        call gx_fill_core
.glyph_18:
        ld l,-2(ix)
        ld h,-1(ix)
        ld (g_x),hl
        ld l,6(ix)
        ld h,7(ix)
        ld (g_y),hl
        ld l,-6(ix)
        ld h,-5(ix)
        ld (g_b),hl
        ld a,12(ix)
        ld (g_c),a
        ld a,13(ix)
        ld (g_m),a
        ld l,14(ix)
        ld h,15(ix)
        ld (g_clip),hl
        ld a,-9(ix)
        ld (g_opaque),a
        call gx_bitmap_core
        ;; x += width + advance
        ld e,10(ix)
        ld d,11(ix)
        push de
        pop iy
        ld a,6(iy)
        add a,-7(ix)
        ld e,a
        ld d,#0
        jr nc,.nc_18
        inc d
.nc_18:
        ld l,-2(ix)
        ld h,-1(ix)
        add hl,de
        ld -2(ix),l
        ld -1(ix),h
        jp .char_18
.done_18:
        call _leave_critical_section
.out_18:
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#10
        add hl,sp
        ld sp,hl
        push bc
        ret
gx_solid_pattern::
        .db 0xff

;; ====================================================================== fill
;; HL / C -> A (remainder).
gx_mod16_8::
        xor a
        ld b,#16
.l_20:
        add hl,hl
        rla
        jr c,.sub_20
        cp c
        jr c,.skip_20
.sub_20:
        sub c
.skip_20:
        djnz .l_20
        ret

;; Fill fill_rows with the pattern byte of every row of the clipped rectangle:
;; index = (top - y0) mod len. Preserves nothing.
gx_fill_rows::
        ld a,(fr_len)
        dec a
        jr nz,.rows_pattern
        ;; Solid vertical pattern: build its row image with one block copy.
        ld a,(fr_h)             ; zero means 256 rows
        dec a
        ld c,a
        ld b,#0
        ld hl,(fr_pat)
        ld a,(hl)
        ld de,#fill_rows
        ld (de),a
        ld hl,#fill_rows
        inc de
        ret z                   ; DEC's height-one flag survives these loads
        ldir
        ret
.rows_pattern:
        ld hl,(fr_top)
        ld de,(fr_y0)
        or a
        sbc hl,de
        ld a,(fr_len)
        ld c,a
        call gx_mod16_8
        ld e,a
        ld d,#0
        ld hl,(fr_pat)
        add hl,de
        ld a,(fr_len)
        sub e
        ld c,a
        ld ix,#fill_rows
        ld a,(fr_h)
        ld b,a
        ld a,(fr_m)
        or a
        jr nz,.all_pattern_rows
        ld a,(fr_len)
        cp #3
        jr nc,.all_pattern_rows
        ld b,#2                 ; COPY's repeating-pair path reads only two
.all_pattern_rows:
.rows_22:
        ld a,(hl)
        ld 0(ix),a
        inc ix
        inc hl
        dec c
        jr nz,.nowrap_22
        ld hl,(fr_pat)
        ld a,(fr_len)
        ld c,a
.nowrap_22:
        djnz .rows_22
        ret

;; HL = rect_t; fr_c, fr_m, fr_pat, fr_len, g_clip set.
gx_fill_core::
        push ix
        call gx_fill_body
        pop ix
        ret
gx_fill_body::
        ld a,(fr_len)
        or a
        ret z
        ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld (fr_x0),de
        ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld (fr_y0),de
        ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld (fr_x1),de
        ld e,(hl)
        inc hl
        ld d,(hl)
        ld (fr_y1),de
        ld hl,(fr_x0)
        ld de,(fr_x1)
        call gx_minmax
        ld (fr_x0),hl
        ld (fr_x1),de
        ld hl,(fr_y0)
        ld de,(fr_y1)
        call gx_minmax
        ld (fr_y0),hl
        ld (fr_y1),de
        call gx_make_vis
        ld hl,(fr_x0)
        ld (fr_lo),hl
        ld hl,(fr_x1)
        ld (fr_hi),hl
        ld hl,(fr_y0)
        ld (fr_top),hl
        ld hl,(fr_y1)
        ld (fr_bot),hl
        ld hl,(vis_x0)
        ld bc,#fr_lo
        call gx_upd_max
        ld hl,(vis_x1)
        ld bc,#fr_hi
        call gx_upd_min
        ld hl,(vis_y0)
        ld bc,#fr_top
        call gx_upd_max
        ld hl,(vis_y1)
        ld bc,#fr_bot
        call gx_upd_min
        ld hl,(fr_hi)
        ld de,(fr_lo)
        call gx_cmp_s
        ret c
        ld hl,(fr_bot)
        ld de,(fr_top)
        call gx_cmp_s
        ret c
        or a
        sbc hl,de
        inc hl
        ld (fr_h),hl
        ;; flags: ink bit0, XOR bit1, OR bit2
        ld a,(fr_c)
        and #1
        ld c,a
        ld a,(fr_m)
        cp #1
        jr nz,.not_xor_22
        ld a,c
        or #2
        ld c,a
        jr .flags_22
.not_xor_22:
        cp #2
        jr nz,.flags_22
        ld a,c
        or #4
        ld c,a
.flags_22:
        ld a,c
        ld (fr_flags),a
        and #6
        jp z,gx_fill_copy               ; plain COPY builds each phase itself
        call gx_fill_rows
        ld hl,(fr_lo)
.xloop_22:
        push hl
        ld de,(fr_hi)
        or a
        sbc hl,de
        pop hl
        jr z,.col_22
        jr nc,.end_22
.col_22:
        call gx_fill_pixel_col
        ld hl,(fc_x)
        inc hl
        jr .xloop_22
.end_22:
        ret

;; One pixel column of the clipped rectangle through _next_fill_column
;; (byte pixel operations). HL = x. Needs fill_rows, fr_* and fr_flags.
gx_fill_pixel_col::
        ld (fc_x),hl
        ld de,(fr_x0)
        or a
        sbc hl,de
        ld a,l
        and #7
        ld b,a
        ld a,#0x80
        inc b
        dec b
        jr z,.phase_pc
.ph_pc:
        rrca
        djnz .ph_pc
.phase_pc:
        ld c,a                  ; C = phase
        ld a,(fr_flags)
        ld b,a
        ld h,b
        ld l,c
        push hl                 ; phase (low), flags (high)
        ld hl,#fill_rows
        push hl
        ld hl,(fr_h)
        push hl
        ld de,(fr_top)
        ld hl,(fc_x)
        call _next_fill_column          ; callee removes the arguments
        ret

 ;; Plain COPY fill uses complete pixel columns. A pattern has eight
 ;; horizontal phases, eight columns apart; a solid fill needs one image.
gx_fill_copy::
        ld a,(fr_h)
        ld (fi_h),a
        ld a,(fr_h+1)
        ld (fi_h+1),a
        ld a,(fr_top)
        ld (fi_y),a
        ld a,(fr_len)
        cp #3
        ld a,(fr_flags)
        jr nc,.fc_general_rows
        or #2                   ; one-/two-row patterns repeat a pixel pair
.fc_general_rows:
        ld (fi_flags),a
        ld hl,(fr_lo)
        ld (fc_jlo),hl
        ld hl,(fr_hi)
        ld (fc_jhi),hl
.fc_images:
        ;; solid: one pattern byte, 0x00 or 0xFF
        ld a,(fr_len)
        cp #1
        jr nz,.fc_phases
        ld hl,(fr_pat)
        ld a,(hl)
        or a
        jr z,.fc_solid
        cp #0xff
        jr nz,.fc_phases
.fc_solid:
        ld a,#0x80
        ld (fi_mask),a
        ld a,#1
        ld (fi_step),a
        ld hl,(fc_jlo)
        ld (fi_x),hl
        ld de,(fc_jhi)
        ex de,hl
        or a
        sbc hl,de
        inc hl
        ld (fi_cnt),hl
        call gx_fill_rows
        jp _next_fill_img
.fc_phases:
        ld a,#8
        ld (fc_p),a                     ; phases left (fc_x holds the phase)
        ld hl,#0
        ld (fc_x),hl
.fc_phase:
        ld hl,(fc_jlo)
        ld de,(fc_x)
        add hl,de                       ; first byte column of this phase
        ld (fi_x),hl
        ex de,hl
        ld hl,(fc_jhi)
        or a
        sbc hl,de                       ; jhi - first
        ret c                           ; no more columns
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l
        inc hl                          ; (last-first)/8 + 1
        ld (fi_cnt),hl
        ld a,#8
        ld (fi_step),a
        ld hl,(fi_x)
        ld de,(fr_x0)
        or a
        sbc hl,de
        ld a,l
        and #7
        ld b,a
        ld a,#0x80
        inc b
        dec b
        jr z,.fc_mask
.fc_shift:
        rrca
        djnz .fc_shift
.fc_mask:
        ld (fi_mask),a                  ; bit of the even pixel
        call gx_fill_rows
        call _next_fill_img
        ld hl,(fc_x)
        inc hl
        ld (fc_x),hl
        ld hl,#fc_p
        dec (hl)
        jr nz,.fc_phase
        ret

;; gpx_fill_rectangle(g=HL, r=DE; c, m, pattern, len, clip)
_gpx_fill_rectangle::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        ld a,d
        or e
        jr z,.out_23
        ld l,8(ix)
        ld h,9(ix)
        ld a,h
        or l
        jr z,.out_23
        ld a,10(ix)
        or a
        jr z,.out_23
        call _enter_critical_section
        ld (fr_pat),hl
        ld (fr_len),a
        ld a,6(ix)
        ld (fr_c),a
        ld a,7(ix)
        ld (fr_m),a
        ld l,11(ix)
        ld h,12(ix)
        ld (g_clip),hl
        ex de,hl
        call gx_fill_core
        call _leave_critical_section
.out_23:
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#7
        add hl,sp
        ld sp,hl
        push bc
        ret

;; ====================================================================== pixel
;; gpx_draw_pixel(g=HL, x=DE; y, c, m, clip)
_gpx_draw_pixel::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        call _enter_critical_section
        ld l,10(ix)
        ld h,11(ix)
        ld (g_clip),hl
        push de
        call gx_make_vis
        pop hl                  ; HL = x
        ld e,6(ix)
        ld d,7(ix)
        ld c,8(ix)
        ld b,9(ix)
        call gx_plot
        call _leave_critical_section
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#6
        add hl,sp
        ld sp,hl
        push bc
        ret

;; next_raw(x=HL, y=DE; flags, clip): circle plotting with a clip rectangle.
_next_raw::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        call _enter_critical_section
        ld c,7(ix)
        ld b,8(ix)
        ld (g_clip),bc
        push de
        push hl
        call gx_make_vis
        pop hl
        pop de
        call gx_visible
        jr c,.raw_done_25
        ld a,6(ix)
        push af
        inc sp
        call _next_sample
.raw_done_25:
        call _leave_critical_section
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#3
        add hl,sp
        ld sp,hl
        push bc
        ret


;; ======================================================================= line
;; gpx_draw_line(g=HL, x=DE; y, x1, y1, c, m, pattern, clip) -> A = pattern
;; Params: 6 y, 8 x1, 10 y1, 12 c, 13 m, 14 pattern, 15 clip.
;; Locals: -2 x, -4 y, -6 dx, -8 dy, -10 err, -12 sx, -14 sy, -16 twice.
_gpx_draw_line::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        ld hl,#-16
        add hl,sp
        ld sp,hl
        call _enter_critical_section
        ld -2(ix),e
        ld -1(ix),d
        ld l,6(ix)
        ld h,7(ix)
        ld -4(ix),l
        ld -3(ix),h
        ld l,15(ix)
        ld h,16(ix)
        ld (g_clip),hl
        ;; dx = |x1 - x|, sx = sign
        ld l,8(ix)
        ld h,9(ix)
        ld e,-2(ix)
        ld d,-1(ix)
        or a
        sbc hl,de
        ld de,#1
        bit 7,h
        jr z,.dx_pos_26
        ld de,#-1
        ld a,l
        cpl
        ld l,a
        ld a,h
        cpl
        ld h,a
        inc hl
.dx_pos_26:
        ld -6(ix),l
        ld -5(ix),h
        ld -12(ix),e
        ld -11(ix),d
        ;; dy = |y1 - y|, sy = sign
        ld l,10(ix)
        ld h,11(ix)
        ld e,-4(ix)
        ld d,-3(ix)
        or a
        sbc hl,de
        ld de,#1
        bit 7,h
        jr z,.dy_pos_26
        ld de,#-1
        ld a,l
        cpl
        ld l,a
        ld a,h
        cpl
        ld h,a
        inc hl
.dy_pos_26:
        ld -8(ix),l
        ld -7(ix),h
        ld -14(ix),e
        ld -13(ix),d
        ;; solid horizontal / vertical lines are one blit
        ld a,14(ix)
        cp #255
        jp nz,.bresenham_26
        ld a,-6(ix)
        or -5(ix)
        jr z,.solid_26
        ld a,-8(ix)
        or -7(ix)
        jp nz,.bresenham_26
.solid_26:
        call gx_make_vis
        ld l,-2(ix)
        ld h,-1(ix)
        ld e,8(ix)
        ld d,9(ix)
        call gx_minmax             ; HL = min(x, x1)
        ld (ax_pos),hl
        ld (g_x),hl
        ld l,-6(ix)
        ld h,-5(ix)
        inc hl
        ld (ax_len),hl
        ld hl,(vis_x0)
        ld (ax_lo),hl
        ld hl,(vis_x1)
        ld (ax_hi),hl
        call gx_axis
        jp c,.line_done_26
        ld hl,(ax_cnt)
        ld (_next_blit_args+6),hl
        ld hl,(ax_off)
        ld de,(g_x)
        add hl,de
        ld (_next_blit_args+9),hl
        ld l,-4(ix)
        ld h,-3(ix)
        ld e,10(ix)
        ld d,11(ix)
        call gx_minmax             ; HL = min(y, y1)
        ld (ax_pos),hl
        ld (g_y),hl
        ld l,-8(ix)
        ld h,-7(ix)
        inc hl
        ld (ax_len),hl
        ld hl,(vis_y0)
        ld (ax_lo),hl
        ld hl,(vis_y1)
        ld (ax_hi),hl
        call gx_axis
        jp c,.line_done_26
        ld a,(ax_cnt)
        ld (_next_blit_args+8),a
        ld hl,(ax_off)
        ld de,(g_y)
        add hl,de
        ld a,l
        ld (_next_blit_args+11),a
        ld hl,#gx_solid_table
        ld (_next_blit_args+0),hl
        xor a
        ld (_next_blit_args+2),a
        ld (_next_blit_args+3),a
        ld (_next_blit_args+4),a
        ld a,#0x80
        ld (_next_blit_args+5),a
        ld a,12(ix)
        ld (g_c),a
        ld a,13(ix)
        ld (g_m),a
        call gx_ink_kind
        ld (_next_blit_args+12),a
        xor a
        ld (_next_blit_args+13),a
        call _next_blit
        jp .line_done_26
.bresenham_26:
        ld l,-6(ix)
        ld h,-5(ix)
        ld e,-8(ix)
        ld d,-7(ix)
        or a
        sbc hl,de
        ld -10(ix),l
        ld -9(ix),h             ; err = dx - dy
        call gx_make_vis
.line_loop_26:
        ld a,14(ix)
        and #1
        jr z,.zero_bit_26
        ld l,-2(ix)
        ld h,-1(ix)
        ld e,-4(ix)
        ld d,-3(ix)
        ld c,12(ix)
        ld b,13(ix)
        call gx_plot
        jr .stepped_26
.zero_bit_26:
        ld a,13(ix)
        or a
        jr nz,.stepped_26          ; only BM_CPY paints pattern zeroes
        ld l,-2(ix)
        ld h,-1(ix)
        ld e,-4(ix)
        ld d,-3(ix)
        ld a,12(ix)
        xor #1
        ld c,a
        ld b,#0
        call gx_plot
.stepped_26:
        ld a,14(ix)
        rrca
        ld 14(ix),a
        ld a,-2(ix)
        cp 8(ix)
        jr nz,.go_26
        ld a,-1(ix)
        cp 9(ix)
        jr nz,.go_26
        ld a,-4(ix)
        cp 10(ix)
        jr nz,.go_26
        ld a,-3(ix)
        cp 11(ix)
        jp z,.line_done_26
.go_26:
        ld l,-10(ix)
        ld h,-9(ix)
        add hl,hl
        ld -16(ix),l
        ld -15(ix),h            ; twice = 2 * err
        ld e,-8(ix)
        ld d,-7(ix)
        ld hl,#0
        or a
        sbc hl,de               ; HL = -dy
        ld e,-16(ix)
        ld d,-15(ix)
        call gx_cmp_s              ; -dy < twice ?
        jr nc,.no_x_26
        ld l,-10(ix)
        ld h,-9(ix)
        ld e,-8(ix)
        ld d,-7(ix)
        or a
        sbc hl,de
        ld -10(ix),l
        ld -9(ix),h             ; err -= dy
        ld l,-2(ix)
        ld h,-1(ix)
        ld e,-12(ix)
        ld d,-11(ix)
        add hl,de
        ld -2(ix),l
        ld -1(ix),h             ; x += sx
.no_x_26:
        ld l,-16(ix)
        ld h,-15(ix)
        ld e,-6(ix)
        ld d,-5(ix)
        call gx_cmp_s              ; twice < dx ?
        jp nc,.line_loop_26
        ld l,-10(ix)
        ld h,-9(ix)
        add hl,de
        ld -10(ix),l
        ld -9(ix),h             ; err += dx
        ld l,-4(ix)
        ld h,-3(ix)
        ld e,-14(ix)
        ld d,-13(ix)
        add hl,de
        ld -4(ix),l
        ld -3(ix),h             ; y += sy
        jp .line_loop_26
.line_done_26:
        call _leave_critical_section
        ld a,14(ix)
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#11
        add hl,sp
        ld sp,hl
        push bc
        ret
gx_solid_table::
        .db 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
        .db 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
        .db 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
        .db 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
        .db 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
        .db 0xff

;; ================================================================ soft sprites
;; gpx_show_sprite_soft(g=HL, s=DE): save the Layer 2 background, then draw.
;; Locals: -1 xx, -2 yy, -3 w, -4 h.
_gpx_show_sprite_soft::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        ld a,d
        or e
        jp z,.show_out_28
        push de
        pop iy
        ld l,4(iy)
        ld h,5(iy)
        ld a,h
        or l
        jp z,.show_out_28
        call _enter_critical_section
        ld hl,#-8
        add hl,sp
        ld sp,hl
        ld l,4(iy)
        ld h,5(iy)
        inc hl
        ld a,(hl)
        cp #17
        jr c,.w_ok_28
        ld a,#16
.w_ok_28:
        ld -3(ix),a
        inc hl
        ld a,(hl)
        cp #17
        jr c,.h_ok_28
        ld a,#16
.h_ok_28:
        ld -4(ix),a
        ld l,6(iy)
        ld h,7(iy)
        ld a,h
        or l
        jp z,.draw_28
        ld (hl),#1
        inc hl
        ld a,-3(ix)
        ld (hl),a
        inc hl
        ld a,-4(ix)
        ld (hl),a
        inc hl
        add a,a
        ld (hl),a
        inc hl
        ld (hl),#0
        inc hl
        ld a,-4(ix)
        add a,a
        ld b,a
        xor a
.clear_28:
        ld (hl),a
        inc hl
        djnz .clear_28
        ;; Save the background as a 1bpp image. A Layer 2 pixel is one byte and
        ;; a column is contiguous, so the whole sprite is read byte by byte
        ;; inside ONE overlay session (no per-pixel call). Everything run or
        ;; read while the overlay is on lives at 4000 and above.
        ;; Locals: -5 first sprite row, -6 row count, -7 first screen row,
        ;; -8 sprite column.
        ld l,2(iy)
        ld h,3(iy)              ; py
        ld a,h
        or a
        jr nz,.py_hi_28
        ld -5(ix),a             ; first row 0
        ld a,l
        neg                     ; 256 - py (0 = the whole column fits)
        jr z,.rn_h_28
        cp -4(ix)
        jr nc,.rn_h_28
        ld -6(ix),a
        jr .rows_28
.rn_h_28:
        ld a,-4(ix)
        ld -6(ix),a
        jr .rows_28
.py_hi_28:
        bit 7,h
        jp z,.draw_28           ; py >= 256: nothing to save
        ld a,h
        inc a
        jp nz,.draw_28          ; py < -256
        ld a,l
        neg                     ; first row = -py
        cp -4(ix)
        jp nc,.draw_28          ; entirely above the screen
        ld -5(ix),a
        ld b,a
        ld a,-4(ix)
        sub b
        ld -6(ix),a
.rows_28:
        ld a,-5(ix)
        or a
        ld a,#0
        jr nz,.y0_28
        ld a,2(iy)
.y0_28:
        ld -7(ix),a
        ld bc,#0x123b
        ld a,#7                 ; Layer 2 read/write overlay at 0000
        out (c),a
        ld -8(ix),#0
.col_28:
        ld l,0(iy)
        ld h,1(iy)
        ld e,-8(ix)
        ld d,#0
        add hl,de               ; screen x
        ld a,h
        cp #2
        jr nc,.col_next_28      ; off the screen, left or right
        cp #1
        jr nz,.x_ok_28
        ld a,l
        cp #0x40
        jr nc,.col_next_28      ; x >= 320
.x_ok_28:
        ld a,#0xff              ; whole 8-bit pixel
        ld (.px_and_28+1),a     ; patched into the row loop below
        ld a,h
        add a,a
        add a,a
        ld b,a
        ld a,l
        rlca
        rlca
        and #3
        or b
        or #0x10
        ld d,a                  ; framebuffer bank offset of this column
        ld a,l
        and #0x3f
        ld h,a
        ld l,-7(ix)             ; HL = byte offset of the first row
        push hl
        ld bc,#0x123b
        ld a,d
        out (c),a
        ld l,6(iy)              ; buffer = bg + 5 + 2 * first row + (column >> 3)
        ld h,7(iy)
        ld de,#5
        add hl,de
        ld a,-5(ix)
        add a,a
        ld e,a
        ld d,#0
        add hl,de
        ld a,-8(ix)
        rrca
        rrca
        rrca
        and #1
        ld e,a
        ld d,#0
        add hl,de
        ex de,hl                ; DE = buffer
        ld a,-8(ix)
        and #7
        ld b,a
        ld a,#0x80
        inc b
        dec b
        jr z,.mask_28
.shift_28:
        rrca
        djnz .shift_28
.mask_28:
        ld c,a                  ; C = this column's bit
        ld b,-6(ix)             ; B = rows
        pop hl
.row_28:
        ld a,(hl)
.px_and_28:
        and #0xff               ; whole pixel (patched per column)
        jr nz,.white_28
        ld a,(de)
        or c
        ld (de),a
.white_28:
        inc l                   ; next row of the column
        inc de
        inc de
        djnz .row_28
.col_next_28:
        inc -8(ix)
        ld a,-8(ix)
        cp -3(ix)
        jp c,.col_28
        ld bc,#0x123b
        ld a,#2
        out (c),a               ; kernel mapped again before anything is called
.draw_28:
        ld l,0(iy)
        ld h,1(iy)
        ld (g_x),hl
        ld l,2(iy)
        ld h,3(iy)
        ld (g_y),hl
        ld l,4(iy)
        ld h,5(iy)
        ld (g_b),hl
        ld l,8(iy)
        ld h,9(iy)
        ld (g_clip),hl
        ld a,#1
        ld (g_c),a
        xor a
        ld (g_m),a
        ld (g_opaque),a
        call gx_bitmap_core
        call _leave_critical_section
.show_out_28:
        ld sp,ix
        pop iy
        pop ix
        ret

;; gpx_hide_sprite_soft(g=HL, s=DE): restore the saved background.
_gpx_hide_sprite_soft::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        ld a,d
        or e
        jr z,.hide_out_29
        push de
        pop iy
        call _enter_critical_section
        ld l,0(iy)
        ld h,1(iy)
        ld (g_x),hl
        ld l,2(iy)
        ld h,3(iy)
        ld (g_y),hl
        ld l,6(iy)
        ld h,7(iy)
        ld (g_b),hl
        ld l,8(iy)
        ld h,9(iy)
        ld (g_clip),hl
        ld a,#1
        ld (g_c),a
        ld (g_opaque),a
        xor a
        ld (g_m),a
        call gx_bitmap_core
        call _leave_critical_section
.hide_out_29:
        ld sp,ix
        pop iy
        pop ix
        ret
