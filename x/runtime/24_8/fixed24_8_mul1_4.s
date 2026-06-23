        ; fixed24_8_mul1_4.s
        ;
        ; Signed 24.8 fixed-point multiply by exact 1/4.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_mul1_4
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_mul1_4

        .area   _CODE

        ; inputs:  DE:HL = a
        ; outputs: DE:HL = a * 1/4, matching generic multiply shift semantics
_fixed24_8_mul1_4::
        sra     h
        rr      l
        rr      d
        rr      e
        sra     h
        rr      l
        rr      d
        rr      e
        ret
