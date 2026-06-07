        ; time.s
        ;
        ; time() for the xcc Z80 libc, in assembly.  Reads the wall clock via
        ; the platform hook __sys_gettimeofday and returns the whole-seconds field.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module time
        .optsdcc -mz80 sdcccall(1)


        .globl  _time
        .globl  ___sys_gettimeofday

        .area   _DATA
__time_ts:
        .ds     8                       ; scratch struct timespec for the hook

        .area   _CODE

        ; _time
        ; inputs:  HL = time_t *timer (may be NULL)
        ; outputs: DE:HL = current time (DE = low16, HL = high16); *timer set
        ; clobbers: AF, BC, DE, HL
_time::
        ld      a,h
        or      l
        push    af                      ; remember whether timer == NULL (Z)
        push    hl                      ; save timer
        ld      hl,#__time_ts
        call    ___sys_gettimeofday           ; __time_ts.tv_sec = current seconds
        pop     hl                      ; HL = timer
        pop     af                      ; Z set if timer was NULL
        jr      z,time_no_store
        ld      a,(__time_ts)
        ld      (hl),a
        inc     hl
        ld      a,(__time_ts + 1)
        ld      (hl),a
        inc     hl
        ld      a,(__time_ts + 2)
        ld      (hl),a
        inc     hl
        ld      a,(__time_ts + 3)
        ld      (hl),a
time_no_store:
        ld      de,(__time_ts)
        ld      hl,(__time_ts + 2)
        ret
