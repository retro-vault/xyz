        ; Schedule an image prepared by the common XPRG loader.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _process_load_finish
        .optsdcc -mz80 sdcccall(1)
        .globl  __process_load_finish
        .globl  _process_start
        .globl  __image_transfer
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .area   _CODE

        ; inputs: ix = shared loader frame
        ; outputs: de = process or zero; preserves ix and iy
        ; clobbers: af, bc, de, hl
__process_load_finish::
        bit     0, 7(ix)
        jr      z, .stack
        ld      a, 68(ix)
        cp      24(ix)
        jr      nz, .invalid
        ld      a, 69(ix)
        cp      25(ix)
        jr      nz, .invalid
.stack:
        ld      l, 28(ix)
        ld      h, 29(ix)
        ld      bc, #22
        add     hl, bc
        jr      c, .invalid
        push    hl
        push    ix
        pop     hl
        ld      bc, #40
        add     hl, bc
        ld      e, 76(ix)
        ld      d, 77(ix)
        call    _enter_critical_section
        call    _process_start
        ld      a, d
        or      e
        call    nz, __image_transfer
        call    _leave_critical_section
        ld      a, #5
        ret
.invalid:
        ld      a, #4
        ld      de, #0
        ret
