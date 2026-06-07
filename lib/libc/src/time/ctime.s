        ; ctime.s
        ;
        ; ctime() for the xcc Z80 libc, in assembly.  Thin wrapper over
        ; ctime_r that targets a single static 26-byte buffer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ctime
        .optsdcc -mz80 sdcccall(1)


        .globl  _ctime
        .globl  _ctime_r

        .area   _DATA
__ctime_buf:
        .ds     26

        .area   _CODE

        ; _ctime
        ; inputs:  HL = const time_t *timer
        ; outputs: DE = &__ctime_buf, filled
_ctime::
        ld      de,#__ctime_buf
        jp      _ctime_r
