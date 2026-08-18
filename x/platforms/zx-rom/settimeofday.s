        ; sys_settimeofday.s  (sys backend: zx-rom)
        ;
        ; The 48K target has no wall clock.

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _settimeofday

        .area   _CODE

_settimeofday::
        ld      de,#0xffff
        ret
