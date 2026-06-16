        ; sys_gettimeofday.s  (sys backend: sim (in-RAM simulator))
        ;
        ; Platform clock hook.  On the "none" backend there is no real-time
        ; clock, so this empty shell reports the epoch (0 = 1970-01-01
        ; 00:00:00 UTC) and succeeds.  An operating system provides its own
        ; lib/sys/<os>/sys_gettimeofday that reads the actual hardware clock.
        ;
        ; All sys-layer hooks use the __sys_ prefix.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)


        .globl  _gettimeofday

        .area   _CODE

        ; _gettimeofday  (C: gettimeofday)
        ; inputs:  HL = struct timespec *tv  (tv_sec[0..3], tv_nsec[4..7])
        ; outputs: DE = 0 on success; *tv = {0, 0}
        ; clobbers: AF, B, HL
_gettimeofday::
        xor     a
        ld      b,#8                    ; 8 bytes: tv_sec + tv_nsec
gtod_zero:
        ld      (hl),a
        inc     hl
        dec     b
        jr      nz,gtod_zero
        ld      de,#0x0000              ; return 0
        ret
