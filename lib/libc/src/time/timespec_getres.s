        ;; timespec_getres.s
        ;; Split from timespec_get.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module timespec_getres
        .optsdcc -mz80 sdcccall(1)

        .globl  _timespec_getres

        .area   _CODE
_timespec_getres::
        ld      a,e
        dec     a
        or      d                       ; base == 1 (TIME_UTC) ?
        jr      nz,timespec_res_bad
        ; tv_sec = 0 (4 bytes, little-endian time_t)
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ; tv_nsec = 1 (4 bytes, little-endian long)
        ld      (hl),#1
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ld      de,#1                   ; TIME_UTC
        ret
timespec_res_bad:
        ld      de,#0
        ret
