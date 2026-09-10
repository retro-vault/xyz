        ; sys_gettimeofday.s  (sys backend: zx-esxdos)
        ;
        ; The 48K target has no wall clock.

        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _gettimeofday

        .area   _CODE

        ; _gettimeofday
        ; inputs: HL = timespec pointer (sdcccall(1)).
        ; outputs: DE = -1; no wall clock is supplied.
        ; clobbers: de.
_gettimeofday::
        ld      de,#0xffff
        ret
