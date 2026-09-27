        ; Dispatch due timers from the interrupt scheduler.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _tmr_chain
        .optsdcc -mz80 sdcccall(1)
        .globl  __tmr_chain
        .globl  __tmr_first
        .globl  __bank_current
        .globl  __bank_map
        .equ    TIMER_BANK, 11
        .area   _CODE

        ; inputs: none; called with interrupts disabled
        ; outputs: none; restores the interrupted execution bank
        ; clobbers: af, bc, de, hl; preserves ix and iy
__tmr_chain::
        push    ix
        push    iy
        ld      hl, (__tmr_first)
.loop:
        ld      a, h
        or      l
        jr      z, .done
        push    hl
        ld      de, #9
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
        push    hl
        pop     iy                      ; callback address
        ld      hl, #2
        add     hl, sp                  ; timer saved by the list walk
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ex      de, hl
        ld      de, #TIMER_BANK
        add     hl, de
        ld      a, (hl)
        cp      #0xff
        jr      z, .invoke_iy
        ld      c, a
        ld      a, (__bank_current)
        cp      c
        jr      z, .invoke_iy
        push    af                      ; interrupted bank, fixed stack
        ld      a, c
        call    __bank_map
        call    .invoke_iy
        pop     af
        jp      __bank_map              ; restore before the next timer
.invoke_iy:
        jp      (iy)
