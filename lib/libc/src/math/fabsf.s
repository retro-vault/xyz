        ; fabsf.s — absolute value (clear the sign bit).  float/double/long
        ; double are all 32-bit here, so one body serves fabsf/fabs/fabsl.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fabsf
        .optsdcc -mz80 sdcccall(1)
        .globl  _fabsf
        .globl  _fabs
        .globl  _fabsl
        .area   _CODE
        ; HL:DE = x -> HL:DE = |x|
_fabs::
_fabsl::
_fabsf::
        res     7,h
        ret
