        ; clock.s
        ;
        ; clock() for the xcc Z80 libc, in assembly.  With CLOCKS_PER_SEC == 1
        ; the processor-time clock is simply the wall clock read through the
        ; platform hook __sys_gettimeofday.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module clock
        .optsdcc -mz80 sdcccall(1)


        .globl  _clock
        .globl  ___sys_gettimeofday

        .area   _DATA
__clock_ts:
        .ds     8                       ; scratch struct timespec for the hook

        .area   _CODE

        ; _clock
        ; outputs: DE:HL = wall seconds (CLOCKS_PER_SEC == 1)
        ; clobbers: AF, BC, DE, HL
_clock::
        ld      hl,#__clock_ts
        call    ___sys_gettimeofday
        ld      de,(__clock_ts)
        ld      hl,(__clock_ts + 2)
        ret
