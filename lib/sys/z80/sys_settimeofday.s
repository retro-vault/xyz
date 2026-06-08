        ; sys_settimeofday.s  (sys backend: generic z80 / bare metal)
        ;
        ; Minimal clock-set hook stub. Ignores the request and succeeds.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module sys_settimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_settimeofday

        .area   _CODE

___sys_settimeofday::
        ld      de,#0x0000
        ret
