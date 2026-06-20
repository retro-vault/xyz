        ;; settimeofday.s  (sys backend: emu)

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _settimeofday

        .area   _CODE
_settimeofday::
        ld      de,#0xffff
        ret
