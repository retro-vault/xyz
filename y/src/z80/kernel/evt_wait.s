        ; Suspend the caller until one binary event is consumed by the scheduler.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih

        .module evt_wait
        .optsdcc -mz80 sdcccall(1)
        .globl  _evt_wait
        .globl  _thread_current
        .globl  _thread_first_running
        .globl  _thread_first_waiting
        .globl  __thread_lswitch
        .equ    THREAD_WAIT,       17
        .equ    THREAD_NUM_EVENTS, 19
        .equ    STATE_WAITING,      2
        .area   _CODE

        ; inputs: hl = live event; caller is a thread with interrupts enabled
        ; outputs: none; signal is consumed when the scheduler wakes the caller
        ; clobbers: af, bc, de, hl; preserves ix and iy; no stack arguments
        ; The pushed handle forms a one-element array on the sleeping stack.
        ; Publishing the waiting queue is protected by __thread_lswitch. A
        ; signal before publication stays set and is consumed on the next IRQ.
_evt_wait::
        push    ix
        push    hl
        ld      ix, (_thread_current)
        ld      hl, #0
        add     hl, sp
        ld      THREAD_WAIT(ix), l
        ld      THREAD_WAIT+1(ix), h
        ld      THREAD_NUM_EVENTS(ix), #1
        ld      bc, #1                  ; yield after changing queues
        push    bc
        ld      a, #STATE_WAITING
        push    af
        inc     sp                      ; retain only state byte
        push    ix
        ld      de, #_thread_first_waiting
        ld      hl, #_thread_first_running
        call    __thread_lswitch
        pop     hl                      ; release the borrowed wait array
        pop     ix
        ret
