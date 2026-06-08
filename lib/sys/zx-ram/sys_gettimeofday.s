        ; sys_gettimeofday.s  (sys backend: zx-ram)
        ;
        ; Minimal clock hook stub. Reports the Unix epoch and succeeds.

        .module sys_gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  ___sys_gettimeofday

        .area   _CODE

___sys_gettimeofday::
        xor     a
        ld      b,#8
gtod_zero:
        ld      (hl),a
        inc     hl
        dec     b
        jr      nz,gtod_zero
        ld      de,#0x0000
        ret
