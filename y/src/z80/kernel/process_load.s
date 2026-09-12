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
_process_load::
        xor     a
        jp      __image_load
