        ; sys_gettimeofday.s  (sys backend: zx-rom)
        ;
        ; Minimal clock hook stub. Reports the Unix epoch and succeeds.

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _gettimeofday

        .area   _CODE

_gettimeofday::
        xor     a
        ld      b,#8
gtod_zero:
        ld      (hl),a
        inc     hl
        dec     b
        jr      nz,gtod_zero
        ld      de,#0x0000
        ret
