        ; clock.s
        ;
        ; clock() for the xcc Z80 libc, in assembly.  With CLOCKS_PER_SEC == 1
        ; the processor-time clock is simply the wall clock read through the
        ; platform hook gettimeofday.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module clock
        .optsdcc -mz80 sdcccall(1)


        .globl  _clock
        .globl  _gettimeofday

        .area   _CODE

        ; _clock
        ; outputs: DE:HL = wall seconds (CLOCKS_PER_SEC == 1)
        ; clobbers: AF, BC, DE, HL
_clock::
        push    ix
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        push    hl
        ld      ix,#0
        add     ix,sp
        push    ix
        pop     hl
        call    _gettimeofday
        ld      e,0(ix)
        ld      d,1(ix)
        ld      l,2(ix)
        ld      h,3(ix)
        ld      sp,ix
        pop     ix
        ret
