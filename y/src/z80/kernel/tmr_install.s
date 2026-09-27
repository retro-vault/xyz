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
        .globl  __bank_current

        .area   _CODE

        ; inputs: hl = hook, de = period, owner at sp+2; removes owner
        ; outputs: de = timer or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; Banked hooks belong to the currently mapped execution bank.
_tmr_install::
        call    _enter_critical_section
        call    __frame_ix
        push    hl
        push    de
        ld      l, 4(ix)
        ld      h, 5(ix)
        push    hl
        ld      de, #12
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
        inc     hl
        ld      a, d                    ; hook high byte
        cp      #0xc0
        ld      a, #0xff
        jr      c, .bank_ready
        ld      a, (__bank_current)
.bank_ready:
        ld      (hl), a
        pop     de
.done:
        call    _leave_critical_section
        pop     ix
        pop     hl
        pop     bc
        jp      (hl)
