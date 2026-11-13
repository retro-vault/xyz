        ; Calibrate the Kempston mouse cursor position.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mouse_calibrate
        .optsdcc -mz80 sdcccall(1)

        .globl  _mouse_calibrate
        .globl  __critical_call
        .globl  __mouse_cursor
        .globl  __mouse_hardware

        .equ    KMP_X_PORT, 0xfbdf

        .area   _CODE

        ; inputs: HL = initial x, DE = initial y; preserves ix and iy.
_mouse_calibrate::
        call    __critical_call
        ld      (__mouse_cursor),hl
        ld      (__mouse_cursor+2),de
        ld      bc, #KMP_X_PORT
        in      a, (c)
        ld      (__mouse_hardware), a
        ld      b, #0xff                ; BC = Kempston Y port 0xffdf
        in      a, (c)
        ld      (__mouse_hardware+1), a
        ret
