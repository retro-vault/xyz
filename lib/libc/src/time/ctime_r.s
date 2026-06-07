        ; ctime_r.s
        ;
        ; ctime_r() for the xcc Z80 libc, in assembly.  Equivalent to
        ; asctime_r(localtime_r(timer, &tmp), buf); local == UTC.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ctime_r
        .optsdcc -mz80 sdcccall(1)


        .globl  _ctime_r
        .globl  _localtime_r
        .globl  _asctime_r

        .area   _DATA
__ctime_tm:
        .ds     18                      ; scratch struct tm

        .area   _CODE

        ; _ctime_r
        ; inputs:  HL = const time_t *timer, DE = char *buf
        ; outputs: DE = buf
_ctime_r::
        push    de                      ; save buf
        ld      de,#__ctime_tm
        call    _localtime_r            ; __ctime_tm = *timer broken down
        ld      hl,#__ctime_tm
        pop     de                      ; DE = buf
        jp      _asctime_r
