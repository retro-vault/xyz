        ; Dispatch due timers from the interrupt scheduler.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _tmr_chain
        .optsdcc -mz80 sdcccall(1)
        .globl  __tmr_chain
        .globl  __tmr_first
        .area   _CODE

__tmr_chain::
        push    ix
        push    iy
        ld      hl, (__tmr_first)
.loop:
        ld      a, h
        or      l
        jr      z, .done
        push    hl
        ld      de, #8
        add     hl, de
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      a, d
        or      e
        jr      nz, .decrement
        ld      bc, #-3
        add     hl, bc
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     hl
        ld      (hl), e
        inc     hl
        ld      (hl), d
        ld      bc, #-5
        add     hl, bc
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ex      de, hl
        call    .invoke
        jr      .next
.decrement:
        dec     de
        ld      (hl), d
        dec     hl
        ld      (hl), e
.next:
        pop     hl
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ex      de, hl
        jr      .loop
.done:
        pop     iy
        pop     ix
        ret
.invoke:
        jp      (hl)
