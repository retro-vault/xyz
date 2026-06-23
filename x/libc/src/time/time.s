        ; time.s
        ;
        ; time() for the xcc Z80 libc, in assembly.  Reads the wall clock via
        ; the platform hook gettimeofday and returns the whole-seconds field.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module time
        .optsdcc -mz80 sdcccall(1)


        .globl  _time
        .globl  _gettimeofday

        .area   _CODE

        ; _time
        ; inputs:  HL = time_t *timer (may be NULL)
        ; outputs: DE:HL = current time (DE = low16, HL = high16); *timer set
        ; clobbers: AF, BC, DE, HL
_time::
        push    ix
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        push    hl
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      l
        push    af                      ; remember whether timer == NULL (Z)
        push    hl                      ; save timer
        push    ix
        pop     hl
        call    _gettimeofday           ; __time_ts.tv_sec = current seconds
        pop     hl                      ; HL = timer
        pop     af                      ; Z set if timer was NULL
        jr      z,time_no_store
        ld      a,0(ix)
        ld      (hl),a
        inc     hl
        ld      a,1(ix)
        ld      (hl),a
        inc     hl
        ld      a,2(ix)
        ld      (hl),a
        inc     hl
        ld      a,3(ix)
        ld      (hl),a
time_no_store:
        ld      e,0(ix)
        ld      d,1(ix)
        ld      l,2(ix)
        ld      h,3(ix)
        ld      sp,ix
        pop     ix
        ret
