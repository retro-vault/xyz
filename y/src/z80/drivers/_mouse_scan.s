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

        ld      hl, (__mouse_cursor)
        ld      de, (__mouse_hardware)
        ld      bc, #KMP_X_PORT
        in      a, (c)
        ld      (__mouse_hardware), a
        sub     e
        jr      z, .x_done
        jp      p, .x_right
        add     a, l
        jr      c, .x_normal
        xor     a
.x_normal:
        ld      l, a
        jr      .x_done
.x_right:
        add     a, l
        jr      c, .x_max
        cp      #0xff
        jr      c, .x_store
.x_max:
        ld      a, #0xff
.x_store:
        ld      l, a
.x_done:
        ld      b, #0xff                ; BC = Kempston Y port 0xffdf
        in      a, (c)
        ld      (__mouse_hardware+1), a
        sub     d
        jr      z, .y_done
        neg                             ; screen Y grows downward
        jp      p, .y_down
        add     a, h
        jr      c, .y_store
        xor     a
.y_store:
        ld      h, a
        jr      .y_done
.y_down:
        add     a, h
        jr      c, .y_max
        cp      #0xbf
        jr      c, .y_store
.y_max:
        ld      h, #0xbf
.y_done:
        ld      (__mouse_cursor), hl
        ret
