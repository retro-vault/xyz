        ; Install a YOS timer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module tmr_install
        .optsdcc -mz80 sdcccall(1)
        .globl  _tmr_install
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __tmr_first
        .globl  _so_create
        .globl  __frame_ix

        .area   _CODE

        ; hl = hook, de = period, owner at sp+2; removes owner.
_tmr_install::
        call    _enter_critical_section
        call    __frame_ix
        push    hl
        push    de
        ld      l, 4(ix)
        ld      h, 5(ix)
        push    hl
        ld      de, #10
        ld      hl, #__tmr_first
        call    _so_create
        pop     bc
        pop     hl
        ld      a, d
        or      e
        jr      z, .done
        push    de
        ex      de, hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
        ld      (hl), c
        inc     hl
        ld      (hl), b
        inc     hl
        ld      (hl), c
        inc     hl
        ld      (hl), b
        pop     de
.done:
        call    _leave_critical_section
        pop     ix
        pop     hl
        pop     bc
        jp      (hl)
