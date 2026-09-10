        ; Calibrate the Kempston mouse cursor position.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mouse_calibrate
        .optsdcc -mz80 sdcccall(1)

        .globl  _mouse_calibrate
        .globl  __mouse_cursor
        .globl  __mouse_hardware

        .equ    KMP_X_PORT, 0xfbdf
        .equ    KMP_Y_PORT, 0xffdf

        .area   _CODE

        ; inputs: A = initial x, L = initial y; preserves ix and iy.
_mouse_calibrate::
        ld      c,a                     ; c = x
        ld      b,l                     ; b = y
        ; calibrate
        ; input:  b=start y, c=start x (hint:center)
        ; affects: a, flags, hl, bc
.kmp_calib_raw:
        ld      hl,#__mouse_cursor
        ld      a,c                     ; x to a
        ld      (hl),a                  ; to low cursor pos
        inc     hl
        ld      a,b                     ; y to a
        ld      (hl),a                  ; to high cursor pos
        inc     hl
        ld      bc,#KMP_X_PORT
        in      a,(c)                   ; x to a
        ld      (hl),a                  ; and to low hw pos
        inc     hl
        ld      bc,#KMP_Y_PORT
        in      a,(c)                   ; y to a
        ld      (hl),a                  ; to high hw pos
        ret

