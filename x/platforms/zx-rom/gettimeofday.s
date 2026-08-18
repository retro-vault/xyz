        ; sys_gettimeofday.s  (sys backend: zx-rom)
        ;
        ; The 48K target has no wall clock.

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _gettimeofday

        .area   _CODE

_gettimeofday::
        ld      de,#0xffff
        ret
