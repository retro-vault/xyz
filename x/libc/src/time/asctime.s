        ; asctime.s
        ;
        ; asctime() for the xcc Z80 libc, in assembly.  Thin wrapper over
        ; asctime_r that targets a single static 26-byte buffer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module asctime
        .optsdcc -mz80 sdcccall(1)


        .globl  _asctime
        .globl  _asctime_r

        .area   _DATA
__asctime_buf:
        .ds     26

        .area   _CODE

        ; _asctime
        ; inputs:  HL = const struct tm *t
        ; outputs: DE = &__asctime_buf, filled
_asctime::
        ld      de,#__asctime_buf
        jp      _asctime_r
