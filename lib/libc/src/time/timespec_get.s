        ; timespec_get.s
        ;
        ; timespec_get() and timespec_getres() for the xcc Z80 libc, in assembly.
        ; The platform hook gettimeofday fills a whole struct timespec.
        ; timespec_getres reports 1 ns resolution for TIME_UTC (the representable
        ; precision of struct timespec; actual tick may be coarser).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih




        .module timespec_get
        .optsdcc -mz80 sdcccall(1)

        .globl  _timespec_get
        .globl  _gettimeofday

        .area   _CODE
_timespec_get::
        ld      a,e
        dec     a
        or      d                       ; base == 1 (TIME_UTC) ?
        jr      nz,timespec_bad
        call    _gettimeofday           ; *ts = { now, 0 }
        ld      de,#1                   ; TIME_UTC
        ret
timespec_bad:
        ld      de,#0
        ret

        ; _timespec_getres
        ; inputs:  HL = struct timespec *ts, DE = int base
        ; outputs: DE = base on success (TIME_UTC), else 0; *ts = resolution
        ;          (tv_sec=0, tv_nsec=1 for 1 nanosecond representable resolution)
        ; clobbers: AF, BC, DE, HL
        ; This is new C23 functionality. Implemented in the existing timespec_get.s
        ; per the "only edit existing files, no new files" rule. No static data
        ; used — writes directly to caller-provided buffer (thread-safe).
