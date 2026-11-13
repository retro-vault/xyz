        ;; GPL-2.0; gpx_show_sprite / gpx_hide_sprite for the Next. A sprite that
        ;; is at most 16x16, plain or masked 1bpp, and has no clip or a
        ;; full-screen clip becomes one of 8 hardware sprites (no save-under).
        ;; Anything else falls back to the Layer 2 path (gpx_*_sprite_soft).
        .module next_hwfront
        .optsdcc -mz80 sdcccall(1)
        .globl _gpx_show_sprite, _gpx_hide_sprite
        .globl _gpx_show_sprite_soft, _gpx_hide_sprite_soft
        .globl _next_hw_pattern, _next_hw_place
        .globl _enter_critical_section, _leave_critical_section
        .area _BSS
hw_owner:  .ds 16
hw_bitmap: .ds 16
hw_pattern: .ds 257
hw_packet: .ds 5
hw_slot:   .ds 1
hs_stride: .ds 1
hs_enc:    .ds 1
hs_and:    .ds 1
hs_w:      .ds 1
        .area _CODE

;; DE = sprite pointer to look up -> A = slot or 0xFF. Clobbers BC, HL.
hw_find::
        ld hl,#hw_owner
        ld b,#8
        ld c,#0
hwf_next:
        ld a,(hl)
        cp e
        jr nz,hwf_skip
        inc hl
        ld a,(hl)
        dec hl
        cp d
        jr z,hwf_found
hwf_skip:
        inc hl
        inc hl
        inc c
        djnz hwf_next
        ld a,#0xff
        ret
hwf_found:
        ld a,c
        ret

;; A = slot -> HL = &hw_owner[slot]
hw_owner_addr::
        ld l,a
        ld h,#0
        add hl,hl
        ld de,#hw_owner
        add hl,de
        ret

;; IY = sprite. Carry set = not eligible for a hardware sprite.
hw_eligible::
        ld l,4(iy)
        ld h,5(iy)
        ld a,(hl)
        and #0xf0
        cp #0x20
        jr nc,hwe_no
        inc hl
        ld a,(hl)
        cp #17
        jr nc,hwe_no
        inc hl
        ld a,(hl)
        cp #17
        jr nc,hwe_no
        ld l,8(iy)
        ld h,9(iy)
        ld a,h
        or l
        jr z,hwe_yes
        ld e,(hl)               ; x0 <= 0
        inc hl
        ld d,(hl)
        inc hl
        bit 7,d
        jr nz,hwe_x0
        ld a,d
        or e
        jr nz,hwe_no
hwe_x0:
        ld e,(hl)               ; y0 <= 0
        inc hl
        ld d,(hl)
        inc hl
        bit 7,d
        jr nz,hwe_y0
        ld a,d
        or e
        jr nz,hwe_no
hwe_y0:
        ld e,(hl)               ; x1 >= 639
        inc hl
        ld d,(hl)
        inc hl
        bit 7,d
        jr nz,hwe_no
        ld a,d
        cp #2
        jr nc,hwe_x1
        cp #1
        jr c,hwe_no
        ld a,e
        cp #0x3f
        jr c,hwe_no
hwe_x1:
        ld e,(hl)               ; y1 >= 255
        inc hl
        ld d,(hl)
        bit 7,d
        jr nz,hwe_no
        ld a,d
        or a
        jr nz,hwe_yes
        ld a,e
        cp #255
        jr c,hwe_no
hwe_yes:
        or a
        ret
hwe_no:
        scf
        ret

;; Build the 16x16 8-bit pattern of the sprite bitmap at hw_pattern+1.
;; 00 = ink (black), FF = paper (white), E3 = transparent.
hw_build::
        push ix                 ; IX is part of the preserved libgpx ABI
        ld hl,#hw_pattern+1
        ld de,#hw_pattern+2
        ld (hl),#0xe3
        ld bc,#255
        ldir
        ld l,4(iy)
        ld h,5(iy)
        push hl
        pop ix
        ld a,0(ix)
        and #0x0f
        inc a
        ld (hs_stride),a
        ld a,0(ix)
        and #0xf0
        ld (hs_enc),a
        ld a,1(ix)
        ld (hs_w),a
        ld b,2(ix)              ; B = height
        ld de,#5
        add ix,de
        ld de,#hw_pattern+1
hwb_row:
        push bc
        push de
        ld a,(hs_w)
        ld c,a
        ld b,#0                 ; B = column index
hwb_col:
        push bc
        ld c,b
        call hw_pixel
        pop bc
        ld l,b
        ld h,#0
        pop de
        push de
        add hl,de
        ld (hl),a
        inc b
        dec c
        jr nz,hwb_col
        pop de
        ld hl,#16
        add hl,de
        ex de,hl
        ld a,(hs_stride)
        ld c,a
        ld a,(hs_enc)
        or a
        ld a,c
        jr z,hwb_step
        add a,a
hwb_step:
        ld c,a
        ld b,#0
        add ix,bc
        pop bc
        djnz hwb_row
        pop ix
        ret

;; IX = row, C = column -> A = pattern byte. Preserves C.
hw_pixel::
        ld a,c
        rrca
        rrca
        rrca
        and #0x1f
        ld e,a
        ld d,#0
        push ix
        pop hl
        add hl,de
        ld a,c
        and #7
        ld b,a
        ld a,#0x80
        inc b
        dec b
        jr z,hwp_mask
hwp_shift:
        rrca
        djnz hwp_shift
hwp_mask:
        ld b,a
        ld a,(hs_enc)
        or a
        jr nz,hwp_masked
        ld a,(hl)
        and b
        ld a,#0xe3
        ret z
        xor a
        ret
hwp_masked:
        ld a,(hl)
        and b
        ld (hs_and),a
        ld a,(hs_stride)
        ld e,a
        ld d,#0
        add hl,de
        ld a,(hl)
        and b
        jr z,hwp_noink
        xor a
        ret
hwp_noink:
        ld a,(hs_and)
        or a
        ld a,#0xe3
        ret nz
        ld a,#0xff
        ret

;; hw_slot = slot, C = visible, HL = x (Layer 2 pixels), DE = y.
hw_place::
        ld a,(hw_slot)
        ld (hw_packet),a
        bit 7,h
        jr z,hwpl_x
        ld hl,#0
hwpl_x:
        ld a,h
        cp #2
        jr nc,hwpl_hide
        cp #1
        jr c,hwpl_xok
        ld a,l
        cp #0x40
        jr nc,hwpl_hide
hwpl_xok:
        bit 7,d
        jr nz,hwpl_hide
        ld a,d
        or a
        jr z,hwpl_go
hwpl_hide:
        ld c,#0
hwpl_go:
        ld a,c
        ld (hw_packet+1),a
        ld a,l
        ld (hw_packet+2),a
        ld a,h
        ld (hw_packet+3),a
        ld a,e
        ld (hw_packet+4),a
        ld hl,#hw_packet
        jp _next_hw_place

;; gpx_show_sprite(g=HL, s=DE)
_gpx_show_sprite::
        ld a,d
        or e
        ret z
        push iy
        push hl
        push de
        push de
        pop iy
        ld l,4(iy)
        ld h,5(iy)
        ld a,h
        or l
        jp z,hws_done
        call _enter_critical_section
        call hw_find
        ld (hw_slot),a
        call hw_eligible
        jr nc,hws_eligible
        ld a,(hw_slot)
        cp #0xff
        jp z,hws_soft
        ld c,#0                 ; leaving hardware: hide the slot
        ld hl,#0
        ld de,#0
        call hw_place
        ld a,(hw_slot)
        call hw_owner_addr
        xor a
        ld (hl),a
        inc hl
        ld (hl),a
hws_soft:
        call _leave_critical_section
        pop de
        pop hl
        pop iy
        jp _gpx_show_sprite_soft
hws_eligible:
        ld a,(hw_slot)
        cp #0xff
        jr nz,hws_have
        ld de,#0
        call hw_find            ; first free slot
        ld (hw_slot),a
        cp #0xff
        jp z,hws_soft
        call hw_owner_addr
        push iy
        pop de
        ld (hl),e
        inc hl
        ld (hl),d
hws_have:
        ld a,(hw_slot)          ; pattern cache keyed by the bitmap pointer
        ld l,a
        ld h,#0
        add hl,hl
        ld de,#hw_bitmap
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        dec hl
        push hl
        ld l,4(iy)
        ld h,5(iy)
        or a
        sbc hl,de
        pop hl
        jp z,hws_place
        ld e,4(iy)
        ld d,5(iy)
        ld (hl),e
        inc hl
        ld (hl),d
        call hw_build
        ld a,(hw_slot)
        ld (hw_pattern),a
        ld hl,#hw_pattern
        call _next_hw_pattern
hws_place:
        ld l,0(iy)
        ld h,1(iy)
        ld e,2(iy)
        ld d,3(iy)
        ld c,#1
        call hw_place
        call _leave_critical_section
hws_done:
        pop de
        pop hl
        pop iy
        ret

;; gpx_hide_sprite(g=HL, s=DE)
_gpx_hide_sprite::
        ld a,d
        or e
        jp z,_gpx_hide_sprite_soft
        push iy
        push hl
        push de
        call _enter_critical_section
        call hw_find
        cp #0xff
        jr z,hwh_soft
        ld (hw_slot),a
        ld c,#0
        ld hl,#0
        ld de,#0
        call hw_place
        ld a,(hw_slot)
        call hw_owner_addr
        xor a
        ld (hl),a
        inc hl
        ld (hl),a
        call _leave_critical_section
        pop de
        pop hl
        pop iy
        ret
hwh_soft:
        call _leave_critical_section
        pop de
        pop hl
        pop iy
        jp _gpx_hide_sprite_soft
