        ;; YOS ABI 8 has a 50 Hz monotonic tick counter but no Unix wall clock.
        ;; Standard wall-clock calls therefore fail instead of inventing an
        ;; epoch value.

        .module yos_clock
        .optsdcc -mz80 sdcccall(1)
        .globl  _gettimeofday
        .globl  _settimeofday
        .globl  __errno_value
        .area   _CODE
_gettimeofday::
_settimeofday::
        ld      hl,#38                 ; ENOSYS
        ld      (__errno_value),hl
        ld      de,#0xffff
        ret
