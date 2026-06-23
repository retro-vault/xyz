        ; gmtime.s
        ;
        ; gmtime / localtime (local == UTC) for the xcc Z80 libc, in assembly.
        ; Thin wrappers over gmtime_r that target a single static struct tm.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module gmtime
        .optsdcc -mz80 sdcccall(1)


        .globl  _gmtime
        .globl  _localtime
        .globl  _gmtime_r

        .area   _DATA
__gm_buf:   .ds 18          ; static struct tm for gmtime()/localtime()

        .area   _CODE

        ; _gmtime / _localtime
        ; inputs:  HL = const time_t *timer
        ; outputs: DE = &__gm_buf, filled
_localtime::
_gmtime::
        ld      de,#__gm_buf
        jp      _gmtime_r
