        ; Shared transition of a thread between scheduler queues.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _thread_lswitch
        .optsdcc -mz80 sdcccall(1)

        .globl  __thread_lswitch
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  _list_remove
        .globl  _list_insert

        .equ    THREAD_SIZE,           24
        .equ    THREAD_SP,              4
        .equ    THREAD_WAIT,           16
        .equ    THREAD_NUM_EVENTS,     18
        .equ    THREAD_STATE,          19
        .equ    THREAD_PROCESS,        22
        .equ    CONTEXT_SIZE,          22

        .equ    STATE_SUSPENDED,        0
        .equ    STATE_RUNNING,          1
        .equ    STATE_TERMINATED,       4
        .equ    EVENT_SIGNALED,         1

        .area   _CODE

        ; __thread_lswitch, sdcccall(1)
        ; inputs: hl = source head, de = destination head
        ;         thread at sp+2, state byte at sp+4, bool at sp+5
        ; outputs: none; removes five stack bytes
        ; clobbers: af, bc, de, hl; preserves ix and iy
__thread_lswitch::
        push    ix
        ld      ix, #0
        add     ix, sp
        push    hl
        push    de
        call    _enter_critical_section

        ld      e, 4(ix)
        ld      d, 5(ix)
        ld      l, -2(ix)
        ld      h, -1(ix)
        call    _list_remove
        ld      a, d
        or      e
        jr      z, .lswitch_leave

        ld      e, 4(ix)
        ld      d, 5(ix)
        ld      l, -4(ix)
        ld      h, -3(ix)
        call    _list_insert
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      bc, #THREAD_STATE
        add     hl, bc
        ld      a, 6(ix)
        ld      (hl), a

.lswitch_leave:
        call    _leave_critical_section
        ld      a, 7(ix)
        or      8(ix)
        jr      z, .lswitch_return
        halt
.lswitch_return:
        ld      sp, ix
        pop     ix
        pop     hl                      ; return address
        pop     bc                      ; thread
        inc     sp                      ; state byte
        pop     bc                      ; bool
        push    hl
        ret
