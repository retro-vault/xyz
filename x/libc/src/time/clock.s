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
        inc     de
        ld      a,d
        or      e
        dec     de
        jr      z,clock_failed
        ld      e,0(ix)
        ld      d,1(ix)
        ld      l,2(ix)
        ld      h,3(ix)
        jr      clock_cleanup
clock_failed:
        ld      de,#0xffff
        ld      hl,#0xffff
clock_cleanup:
        ld      sp,ix
        ; Discard the eight-byte timeval object before restoring the caller's
        ; frame pointer.  Popping IX directly here used to load tv_sec's low
        ; word into IX, violating both sdcccall(0) and sdcccall(1).
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        pop     ix
        ret
