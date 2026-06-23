        ; srand.s
        ;
        ; srand() for the xcc Z80 libc.  Seeds rand()'s 32-bit state with
        ;     state = (seed << 16) ^ seed ^ 1
        ; which, for a 16-bit seed, is  high16 = seed, low16 = seed ^ 1.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module srand
        .optsdcc -mz80 sdcccall(1)
        .globl  _srand
        .globl  __rand_state
        .area   _CODE
        ; HL = seed (unsigned int)
_srand::
        ld      d,h
        ld      e,l                     ; DE = seed
        ld      a,e
        xor     #1
        ld      e,a                     ; DE = seed ^ 1  (low word)
        ld      (__rand_state),de
        ld      (__rand_state + 2),hl   ; high word = seed
        ret
