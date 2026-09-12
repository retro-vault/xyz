        ; Return the latest timer-sampled Kempston mouse state.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mouse_read
        .optsdcc -mz80 sdcccall(1)

        .globl  _mouse_read
        .globl  __critical_call
        .globl  __mouse_cursor
        .globl  __mouse_changes

        .area   _CODE

        ; _mouse_read, sdcccall(1)
        ; input: hl = four-byte output {x, y, buttons, changed}
        ; outputs: none; consumes accumulated button-change bits
        ; clobbers: af, bc, de, hl; preserves ix and iy
_mouse_read::
        call    __critical_call
        ex      de, hl                  ; destination
        ld      hl, #__mouse_cursor     ; contiguous public snapshot
        ld      bc, #4
        ldir
        xor     a
        ld      (__mouse_changes), a
        ret
