        ; sys_settimeofday.s  (sys backend: zx-esxdos)
        ;
        ; The 48K target has no wall clock.

        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _settimeofday

        .area   _CODE

        ; _settimeofday
        ; inputs: HL = const timespec pointer (sdcccall(1)).
        ; outputs: DE = -1; no wall clock is supplied.
        ; clobbers: de.
_settimeofday::
        ld      de,#0xffff
        ret
