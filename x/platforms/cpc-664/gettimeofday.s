        ; Convert the CPC 300 Hz firmware ticker to whole seconds.

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)
        .globl  _gettimeofday
        .globl  __divulong

KL_TIME_PLEASE  .equ    0xbd0d

        .area   _CODE
_gettimeofday::
        ld      a,h
        or      l
        jr      z,.cpc_gettime_fail
        push    hl
        call    KL_TIME_PLEASE
        ex      de,hl
        ld      bc,#0
        push    bc
        ld      bc,#300
        push    bc
        call    __divulong
        pop     bc
        pop     bc
        pop     bc
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
        inc     bc
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        inc     bc
        xor     a
        ld      (bc),a
        inc     bc
        ld      (bc),a
        inc     bc
        ld      (bc),a
        inc     bc
        ld      (bc),a
        ld      de,#0
        ret
.cpc_gettime_fail:
        ld      de,#0xffff
        ret
