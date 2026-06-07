        ; timespec_get.s
        ;
        ; timespec_get() for the xcc Z80 libc, in assembly.  The platform hook
        ; __sys_gettimeofday fills a whole struct timespec, so for TIME_UTC this is a
        ; thin wrapper that forwards the pointer and reports the base back.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module timespec_get
        .optsdcc -mz80 sdcccall(1)


        .globl  _timespec_get
        .globl  ___sys_gettimeofday

        .area   _CODE

        ; _timespec_get
        ; inputs:  HL = struct timespec *ts, DE = int base
        ; outputs: DE = base on success (TIME_UTC), else 0; ts filled
        ; clobbers: AF, BC, DE, HL
_timespec_get::
        ld      a,e
        dec     a
        or      d                       ; base == 1 (TIME_UTC) ?
        jr      nz,timespec_bad
        call    ___sys_gettimeofday           ; *ts = { now, 0 }
        ld      de,#1                   ; TIME_UTC
        ret
timespec_bad:
        ld      de,#0
        ret
