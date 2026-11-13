        ;; GPL-2.0; libgpx Layer 2 byte raster adapted for banked YOS.
        .module next_sample
        .optsdcc -mz80 sdcccall(1)
        .globl _next_sample, __gpx_plot_raw, _next_raw
        .globl _enter_critical_section, _leave_critical_section
        .area _GPX
;; Register-fed libgpx circle pixel ABI: DE=x, HL=y, A=flags, BC=clip.
__gpx_plot_raw::
        push bc
        push af
        inc sp
        ex de,hl
        call _next_raw          ; callee-cleanup: pops the flags byte and clip
        ret                     ; back to the caller's own return address
;; C ABI: HL=x (0..319), DE=y, one byte flags on stack; A=sample (black=1).
_next_sample::
        push ix
        ld ix,#0
        add ix,sp
        ;; 320x256x8: one byte per pixel, column-major (x * 256 + y).
        ;; Pixels are 0 (black) and 0xFF (white).
        ld a,h
        add a,a
        add a,a
        ld b,a                  ; bit 8 of the column contributes bank bit 2
        ld a,l
        rlca
        rlca
        and #3
        or b
        ld d,a                  ; framebuffer bank offset 0..4
        ld a,l
        and #0x3f
        ld h,a
        ld l,e                  ; y is the low byte inside a column
        call _enter_critical_section
        ld bc,#0x123b
        ld a,#7                 ; Layer 2 read/write overlay at 0000
        out (c),a
        ld a,d
        or #0x10
        out (c),a
        ld a,(hl)
        ld d,a                  ; old byte
        ld a,4(ix)
        bit 2,a
        jr nz,.read
        bit 1,a
        jr nz,.xor
        and #1
        dec a                   ; white flag 0 -> FF; black flag 1 -> 0
        jr .write
.xor:
        ld a,d
        cpl
.write:
        ld (hl),a
.read:
        ld a,d
        or a
        ld d,#0
        jr nz,.sampled
        inc d
.sampled:
        ld bc,#0x123b
        ld a,#2
        out (c),a               ; restore kernel before critical exit
        call _leave_critical_section
        ld a,d
        pop ix
        pop hl
        inc sp                  ; clean byte argument
        jp (hl)
