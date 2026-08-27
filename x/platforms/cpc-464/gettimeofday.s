        ; Convert the CPC 300 Hz firmware ticker to whole seconds.
        ; The reset epoch is zero and can be changed with settimeofday().
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _gettimeofday
        .globl  __divulong

KL_TIME_PLEASE  .equ    0xbd0d

        .area   _CODE

        ; _gettimeofday
        ; inputs: HL = struct timespec pointer
        ; outputs: DE = zero; tv_sec is uptime/300 and tv_nsec is zero
        ; clobbers: af, bc, de, hl, ix

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
