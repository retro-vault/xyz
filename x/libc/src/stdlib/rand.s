        ; rand.s
        ;
        ; rand() for the xcc Z80 libc.  Linear congruential generator with the
        ; classic glibc constants on a 32-bit state:
        ;     state = state * 1103515245 + 12345
        ;     return (state >> 16) & RAND_MAX        (RAND_MAX == 0x7FFF)
        ; The state is exported so srand() can seed it.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rand
        .optsdcc -mz80 sdcccall(1)

        .globl  _rand
        .globl  __mullong
        .globl  __rand_state

        .area   _CODE
_rand::
        ld      de,(__rand_state)       ; DE = low16
        ld      hl,(__rand_state + 2)   ; HL = high16
        ld      bc,#0x41c6              ; multiplier high word (0x41C64E6D)
        push    bc
        ld      bc,#0x4e6d              ; multiplier low word
        push    bc
        call    __mullong               ; DE:HL = state * 1103515245
        pop     bc
        pop     bc
        ; add 12345 to the 32-bit value (DE = low, HL = high)
        ld      bc,#12345
        ex      de,hl                   ; HL = low, DE = high
        add     hl,bc
        jr      nc,rand_noc
        inc     de
rand_noc:
        ex      de,hl                   ; DE = low, HL = high
        ld      (__rand_state),de
        ld      (__rand_state + 2),hl
        ; return (state >> 16) & 0x7FFF = high word masked
        ld      a,h
        and     #0x7f
        ld      d,a
        ld      e,l
        ret
