        ; Install the Spectrum's 0xFF-bus IM2 vector after disk boot.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _im2_init
        .optsdcc -mz80 sdcccall(1)

        .globl  __im2_init
        .globl  __im2_vector
        .globl  __thread_robin

        .area   _CODE

        ; The 48K ULA supplies 0xFF during interrupt acknowledge. I=0x5E
        ; therefore reads the handler word at 0x5EFF.
__im2_init::
        ld      hl, #__thread_robin
        ld      (__im2_vector), hl
        ld      a, #0x5e
        ld      i, a
        im      2
        ret

        .area   _IM2
__im2_vector::
        .ds     2
