        ; Load and schedule an XPRG process through the common loader.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module process_load
        .optsdcc -mz80 sdcccall(1)
        .globl  _process_load
        .globl  __image_load
        .area   _CODE

        ; inputs: hl = path; outputs: de = process or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; A final .sys extension, matched without regard to case, selects the
        ; fixed OS heap. Other process images use a banked user heap.
_process_load::
        ld      a, h
        or      l
        jr      z, .user
        push    hl
        ld      c, #0                   ; native paths are limited to 255 bytes
.length:
        ld      a, (hl)
        or      a
        jr      z, .suffix
        inc     hl
        inc     c
        jr      .length
.suffix:
        ld      a, c
        cp      #4
        jr      c, .restore
        ld      de, #-4
        add     hl, de
        ld      a, (hl)
        cp      #'.
        jr      nz, .restore
        inc     hl
        ld      de, #.sys
        ex      de, hl
        ld      b, #3
.letter:
        ld      a, (de)
        or      #0x20
        cp      (hl)
        jr      nz, .restore
        inc     de
        inc     hl
        djnz    .letter
        pop     hl
        ld      a, #4                  ; process image in fixed OS memory
        jp      __image_load
.restore:
        pop     hl
.user:
        xor     a                      ; process image in banked user memory
        jp      __image_load
.sys:
        .ascii  "sys"
