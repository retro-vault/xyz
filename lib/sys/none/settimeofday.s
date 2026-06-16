        ;; settimeofday.s  (sys backend: none — template, OPTIONAL)
        ;;
        ;; int settimeofday(const struct timespec *tv)
        ;;   HL = tv                            (sdcccall(1))
        ;;   returns DE = 0 on success, 0xFFFF (-1) if the clock is not settable.
        ;;
        ;; Optional: libc links this only if the program calls a clock-set path.
        ;; The template cannot set a clock, so it reports failure.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _settimeofday

        .area   _CODE
_settimeofday::
        ;; TODO: program your RTC from *tv (HL); return 0 on success.
        ld      de,#0xffff              ; -1: clock not settable
        ret
