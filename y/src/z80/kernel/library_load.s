        ; Acquire a private or shared XPRG library.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module library_load
        .optsdcc -mz80 sdcccall(1)
        .globl  _library_load
        .globl  __image_load
        .globl  _process_last_error
        .area   _CODE

        ; inputs: hl = path, de = flags (0 private, 1 shared)
        ; outputs: de = function-pointer table or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
_library_load::
        ld      a, d
        or      a
        jr      nz, .invalid
        ld      a, e
        cp      #2
        jr      nc, .invalid
        add     a, a
        inc     a
        jp      __image_load
.invalid:
        ld      a, #4
        ld      (_process_last_error), a
        ld      de, #0
        ret
