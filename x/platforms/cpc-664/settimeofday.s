        ; Set the CPC firmware ticker from whole seconds.

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)
        .globl  _settimeofday
        .globl  __mullong

KL_TIME_SET     .equ    0xbd10

        .area   _CODE
_settimeofday::
        ld      a,h
        or      l
        jr      z,.cpc_settime_fail
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        ld      h,b
        ld      l,c
        ld      bc,#0
        push    bc
        ld      bc,#300
        push    bc
        call    __mullong
        pop     bc
        pop     bc
        ex      de,hl
        call    KL_TIME_SET
        ld      de,#0
        ret
.cpc_settime_fail:
        ld      de,#0xffff
        ret
