        ;; gettimeofday.s  (sys backend: none — template)
        ;;
        ;; int gettimeofday(struct timespec *tv)
        ;;   HL = tv   (struct timespec { time_t tv_sec; long tv_nsec; }, 8 bytes)
        ;;   fills *tv and returns DE = 0 (ok), or 0xFFFF (-1) if no clock.
        ;;
        ;; This template has no real-time clock, so it reports the Unix epoch
        ;; (tv = {0, 0}).  Fill tv_sec/tv_nsec from your RTC to make <time.h>
        ;; return real wall-clock time.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _gettimeofday

        .area   _CODE
_gettimeofday::
        ;; TODO: write seconds-since-epoch to tv_sec (HL[0..3]) from your RTC.
        ld      b,#8
        xor     a
gtod_zero:
        ld      (hl),a                  ; tv = { 0, 0 }
        inc     hl
        djnz    gtod_zero
        ld      de,#0                   ; success
        ret
