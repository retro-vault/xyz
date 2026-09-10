        ; Shared reclamation of terminated threads and processes.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _thread_cleanup_terminated
        .optsdcc -mz80 sdcccall(1)

        .globl  __thread_cleanup_terminated
        .globl  __heap
        .globl  _thread_current
        .globl  _thread_first_terminated
        .globl  _mem_free_owner
        .globl  _so_destroy
        .globl  _process_reap

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

        ; Reclaim terminated threads except the current interrupt stack.
        ; frame: current -2, next -4, process -6
__thread_cleanup_terminated::
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      hl, #-6
        add     hl, sp
        ld      sp, hl
        ld      hl, (_thread_first_terminated)
        ld      -2(ix), l
        ld      -1(ix), h
.cleanup_loop:
        ld      l, -2(ix)
        ld      h, -1(ix)
        ld      a, h
        or      l
        jr      z, .cleanup_done
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      -4(ix), e
        ld      -3(ix), d
        ld      l, -2(ix)
        ld      h, -1(ix)
        ld      de, #THREAD_PROCESS
        add     hl, de
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      -6(ix), e
        ld      -5(ix), d

        ld      l, -2(ix)
        ld      h, -1(ix)
        ld      de, (_thread_current)
        or      a
        sbc     hl, de
        jr      z, .cleanup_next

        ld      e, -2(ix)
        ld      d, -1(ix)
        ld      hl, #__heap
        call    _mem_free_owner
        ld      e, -2(ix)
        ld      d, -1(ix)
        ld      hl, #_thread_first_terminated
        call    _so_destroy
        ld      l, -6(ix)
        ld      h, -5(ix)
        call    _process_reap

.cleanup_next:
        ld      l, -4(ix)
        ld      h, -3(ix)
        ld      -2(ix), l
        ld      -1(ix), h
        jr      .cleanup_loop
.cleanup_done:
        ld      sp, ix
        pop     ix
        ret
