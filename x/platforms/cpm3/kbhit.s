        ;; kbhit.s -- non-blocking CP/M 3 console status

        .module kbhit
        .optsdcc -mz80 sdcccall(1)

        .globl  _kbhit

        .equ    BDOS,5
        .equ    C_STAT,11

        .area   _CODE

;; Return zero when no console character is ready, nonzero otherwise.
_kbhit::
        push    ix
        push    iy
        ld      c,#C_STAT
        call    BDOS
        ld      e,a
        ld      d,#0
        pop     iy
        pop     ix
        ret
