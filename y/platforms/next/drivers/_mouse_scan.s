        ; Sample Kempston hardware into the logical mouse state.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _mouse_scan
        .optsdcc -mz80 sdcccall(1)

        .globl  __mouse_scan
        .globl  __mouse_cursor
        .globl  __mouse_hardware
        .globl  __mouse_buttons
        .globl  __mouse_changes

        .equ    KMP_BTN_PORT, 0xfadf
        .equ    KMP_X_PORT,   0xfbdf

        .area   _CODE

        ; __mouse_scan, kernel 50 Hz timer callback
        ; outputs: none
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; Button transitions accumulate until _mouse_read consumes them.
__mouse_scan::
        ld      bc, #KMP_BTN_PORT
        in      a, (c)
        cpl
        and     #0x07
        ld      b, a                    ; current buttons
        ld      a, (__mouse_buttons)
        xor     b                       ; transitions this frame
        ld      hl, #__mouse_changes
        or      (hl)                    ; retain unread transitions
        ld      (hl), a
        ld      a, b
        ld      (__mouse_buttons), a

        ld bc,#0xfbdf
        in a,(c)
        ld hl,#__mouse_hardware
        ld e,(hl)
        ld (hl),a
        sub e
        call .delta
        ld hl,(__mouse_cursor)
        add hl,de
        bit 7,h
        jr nz,.x_zero
        ld a,h
        cp #1
        jr c,.x_store
        jr nz,.x_max
        ld a,l
        cp #0x40                ; x < 640
        jr c,.x_store
.x_max:
        ld hl,#319
        jr .x_store
.x_zero:
        ld hl,#0
.x_store:
        ld (__mouse_cursor),hl
        ld bc,#0xffdf
        in a,(c)
        ld hl,#__mouse_hardware+1
        ld e,(hl)
        ld (hl),a
        sub e
        neg
        call .delta
        ld hl,(__mouse_cursor+2)
        add hl,de
        bit 7,h
        jr nz,.y_zero
        ld a,h
        or a
        jr z,.y_store
        ld hl,#255
        jr .y_store
.y_zero:
        ld hl,#0
.y_store:
        ld (__mouse_cursor+2),hl
        ret
.delta:
        ld e,a
        ld d,#0
        bit 7,a
        ret z
        dec d
        ret
