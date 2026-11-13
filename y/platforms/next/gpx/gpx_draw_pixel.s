        .module gpx_draw_pixel_next_l2
        .optsdcc -mz80 sdcccall(1)
        .include "_zxnext-l2-defs.inc"
        .globl _gpx_draw_pixel
        .globl __gpx_plot_raw
        .globl __nxt_map_page
        .globl __nxt_unmap
        .area _CODE

_gpx_draw_pixel::
        exx
        pop bc
        exx
        pop hl
        pop af
        rla
        and #3
        pop bc
        exx
        push bc
        exx
        jp __gpx_plot_raw

;; DE=x, HL=y, BC=clip, A bit0=color, bit1=BM_XOR.
;; Layer 2 320x256x8 stores one palette index per byte.
__gpx_plot_raw:
        push af
        ld a,e
        and #1
        push af                  ; original x parity
        ld a,d
        cp #2
        jr nc,.reject
        jr nz,.bounds_y
        ld a,e
        cp #128
        jr nc,.reject
.bounds_y:
        ld a,h
        or a
        jr nz,.reject
        ;; byte offset = y*320 + (x>>1), 17 bits
        push de
        ld a,l
        ld d,a
        ld e,#0
        ld h,#0
        ld l,a
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,de
        ld a,#0
        adc a,#0
        pop de
        srl d
        rr e
        add hl,de
        adc a,#0
        ld c,a
        ld a,h
        rra
        rra
        and #3
        ld b,a
        ld a,c
        add a,a
        add a,a
        or b
        ld c,a
        ld a,h
        and #0x3f
        ld h,a
        call __nxt_map_page
        pop af                   ; parity
        ld e,a
        pop af                   ; drawing flags
        bit 0,e
        jr nz,.low
        bit 1,a
        jr nz,.xor_hi
        bit 0,a
        jr nz,.black_hi
        ld a,(hl)
        or #0xf0
        ld (hl),a
        jp __nxt_unmap
.black_hi:
        ld a,(hl)
        and #0x0f
        ld (hl),a
        jp __nxt_unmap
.xor_hi:
        ld a,(hl)
        xor #0xf0
        ld (hl),a
        jp __nxt_unmap
.low:
        bit 1,a
        jr nz,.xor_lo
        bit 0,a
        jr nz,.black_lo
        ld a,(hl)
        and #0xf0
        or #0x0f
        ld (hl),a
        jp __nxt_unmap
.black_lo:
        ld a,(hl)
        and #0xf0
        ld (hl),a
        jp __nxt_unmap
.xor_lo:
        ld a,(hl)
        xor #0x0f
        ld (hl),a
        jp __nxt_unmap
.reject:
        pop af
        pop af
        ret
