        ;; fabsf.s — absolute value (clear the sign bit) for float32.
        ;; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fabsf
        .optsdcc -mz80 sdcccall(1)
        .globl  _fabsf
        .area   _CODE
        ;; HL:DE = x -> HL:DE = |x|
_fabsf::
        res     7,h
        ret
