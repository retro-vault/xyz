        ;; gettimeofday.s  (sys backend: emu)

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _gettimeofday

        .area   _CODE
_gettimeofday::
        ld      b,#8
        xor     a
.zero:
        ld      (hl),a
        inc     hl
        djnz    .zero
        ld      de,#0
        ret
