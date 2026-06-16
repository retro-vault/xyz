        ; sys_settimeofday.s  (sys backend: zx-rom)
        ;
        ; Minimal clock-set hook stub. Ignores the request and succeeds.

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _settimeofday

        .area   _CODE

_settimeofday::
        ld      de,#0x0000
        ret
