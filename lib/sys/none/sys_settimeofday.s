        ; sys_settimeofday.s  (sys backend: none / bare metal)
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

        .module sys_settimeofday
        .optsdcc -mz80 sdcccall(1)


        .globl  ___sys_settimeofday

        .area   _CODE

        ; ___sys_settimeofday  (C: __sys_settimeofday)
        ; inputs:  HL = const struct timespec *tv  (ignored)
        ; outputs: DE = 0 on success
        ; clobbers: AF
___sys_settimeofday::
        ld      de,#0x0000              ; return 0; nothing to do
        ret
