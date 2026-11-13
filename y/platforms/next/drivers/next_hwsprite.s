        ;; GPL-2.0; Next hardware sprite helpers for the pointer. Sprite
        ;; coordinates are the 320x256 space, the same as Layer 2.
        .module next_hwsprite
        .optsdcc -mz80 sdcccall(1)
        .globl _next_hw_init, _next_hw_pattern, _next_hw_place
        .globl _enter_critical_section, _leave_critical_section
        .area _CODE
;; Hide all 128 sprites and select the default sprite palette entries.
_next_hw_init::
        ld d,#0
.hide_all:
        ld bc,#0x303b
        ld a,d
        out (c),a
        ld bc,#0x0057
        xor a
        out (c),a
        out (c),a
        out (c),a
        out (c),a
        inc d
        bit 7,d
        jr z,.hide_all
        ret
;; HL = {slot, 256 pattern bytes}: upload one 16x16 8-bit pattern.
_next_hw_pattern::
        push hl
        call _enter_critical_section
        pop hl
        ld a,(hl)
        inc hl
        ld bc,#0x303b
        out (c),a
        ld bc,#0x005b
        otir
        jp _leave_critical_section
;; HL = {slot, visible, x low, x high, y}: position and show/hide a sprite.
_next_hw_place::
        push hl
        call _enter_critical_section
        pop hl
        ld a,(hl)
        inc hl
        ld d,a                  ; slot doubles as pattern number
        ld bc,#0x303b
        out (c),a
        ld e,(hl)               ; visible
        inc hl
        ld a,(hl)               ; x low
        inc hl
        ld bc,#0x0057
        out (c),a               ; attribute 0
        inc hl                  ; at y
        ld a,(hl)
        out (c),a               ; attribute 1
        dec hl                  ; at x high
        ld a,(hl)
        and #1
        out (c),a               ; attribute 2
        ld a,d
        bit 0,e
        jr z,.hidden
        or #0x80
.hidden:
        out (c),a               ; attribute 3
        jp _leave_critical_section
