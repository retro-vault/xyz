        ; sys_settimeofday.s  (sys backend: sim (in-RAM simulator))
        ;
        ; Platform clock-set hook.  On the "none" backend there is no clock to
        ; set, so this empty shell ignores its argument and succeeds.  An
        ; operating system provides its own lib/sys/<os>/sys_settimeofday that
        ; programs the hardware clock.
        ;
        ; All sys-layer hooks use the __sys_ prefix.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)


        .globl  _settimeofday

        .area   _CODE

        ; _settimeofday  (C: settimeofday)
        ; inputs:  HL = const struct timespec *tv  (ignored)
        ; outputs: DE = 0 on success
        ; clobbers: AF
_settimeofday::
        ld      de,#0x0000              ; return 0; nothing to do
        ret
