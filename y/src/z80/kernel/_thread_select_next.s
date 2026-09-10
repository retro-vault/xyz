        ; Shared selection and wakeup pass for the scheduler.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _thread_select_next
        .optsdcc -mz80 sdcccall(1)

        .globl  __thread_select_next
        .globl  __thread_cleanup_terminated
        .globl  _thread_current
        .globl  _thread_first_waiting
        .globl  _thread_first_running
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

        ; __thread_select_next, sdcccall(1)
        ; outputs: de = next runnable thread or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; frame: waiting iterator -2, examined thread -4
__thread_select_next::
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      hl, #-4
        add     hl, sp
        ld      sp, hl
        call    __thread_cleanup_terminated

        ld      hl, (_thread_first_waiting)
        ld      -2(ix), l
        ld      -1(ix), h
.waiting_loop:
        ld      l, -2(ix)
        ld      h, -1(ix)
        ld      a, h
        or      l
        jr      z, .select_runnable
        ld      -4(ix), l
        ld      -3(ix), h
        ld      e, (hl)                 ; save t->next before inspection
        inc     hl
        ld      d, (hl)
        ld      -2(ix), e
        ld      -1(ix), d

        ld      l, -4(ix)
        ld      h, -3(ix)
        ld      de, #THREAD_NUM_EVENTS
        add     hl, de
        ld      c, (hl)
        ld      a, c
        or      a
        jr      z, .waiting_loop
        ld      l, -4(ix)
        ld      h, -3(ix)
        ld      de, #THREAD_WAIT
        add     hl, de
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ex      de, hl                  ; hl = event pointer array
.event_loop:
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     hl
        push    hl
        ex      de, hl
        ld      de, #4
        add     hl, de
        ld      a, (hl)
        pop     hl
        cp      #EVENT_SIGNALED
        jr      z, .wake_thread
        dec     c
        jr      nz, .event_loop
        jr      .waiting_loop

.wake_thread:
        ld      l, -4(ix)
        ld      h, -3(ix)
        ld      de, #THREAD_STATE
        add     hl, de
        ld      (hl), #STATE_RUNNING
        ld      e, -4(ix)
        ld      d, -3(ix)
        ld      hl, #_thread_first_waiting
        call    _list_remove
        ld      e, -4(ix)
        ld      d, -3(ix)
        ld      hl, #_thread_first_running
        call    _list_insert
        jr      .waiting_loop

.select_runnable:
        ld      hl, (_thread_current)
        ld      a, h
        or      l
        jr      z, .select_first
        push    hl
        ld      de, #THREAD_STATE
        add     hl, de
        ld      a, (hl)
        pop     hl
        cp      #STATE_RUNNING
        jr      nz, .select_first
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      a, d
        or      e
        jr      nz, .select_done
.select_first:
        ld      de, (_thread_first_running)
.select_done:
        ld      sp, ix
        pop     ix
        ret
