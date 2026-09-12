        ; Read one queued ZX Spectrum keyboard transition.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module keyboard_read
        .optsdcc -mz80 sdcccall(1)

        .globl  _kbd_read
        .globl  __kbd_buffer
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .equ    BUFSIZE, 32

        .area   _CODE

        ; outputs: L = one-based raw key code, or zero when empty.
        ; Bit 6 is set for a key-down transition and clear for key-up.
_kbd_read::
        call    _enter_critical_section
        ld      a,(#__kbd_buffer+2)       ; a=count
        or      a                       ; is it zero?
        jr      z,.kr_empty             ; no data in buffer
        ; get the char
        ld      hl,(#__kbd_buffer)        ; l=start, h=end
        ld      de,#__kbd_buffer+3        ; de is start of kbd buffer
        ld      h,#0x00                 ; l=start, h=0
        add     hl,de                   ; hl points to correct place
        ld      b,(hl)                  ; get char to b
        ; decrease counter, increase start
        dec     a
        ld      (#__kbd_buffer+2),a
        ld      a,(#__kbd_buffer)         ; a=start
        inc     a
        cp      #BUFSIZE                ; end of buffer?
        jr      nz,.kr_proceed
        xor     a                       ; reset start
.kr_proceed:
        ld      (#__kbd_buffer),a         ; ...and store
        ld      l,b                     ; return char
        ;; return code 0 means no key so key code has
        ;; to be 1 based (i.e. start with 1)
        inc     l
        jr      .kr_end                 ; game over
.kr_empty:
        ld      hl,#0                   ; key not found
.kr_end:
        jp      _leave_critical_section
