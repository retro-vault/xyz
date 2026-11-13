        ; Wake signaled threads and select the next runnable thread.
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
        .equ    THREAD_WAIT,       17
        .equ    THREAD_NUM_EVENTS, 19
        .equ    THREAD_STATE,      20
        .equ    STATE_RUNNING,     1
        .equ    EVENT_SIGNALED,    1
        .area   _CODE

        ; outputs: de = next runnable thread or zero
        ; clobbers: af, bc, de, hl, ix; preserves iy
        ; IX is the examined thread; its next link is saved on stack.
        ; Only __thread_robin calls this private routine. It restores IX from
        ; the selected context, so saving the interrupted IX again is wasted.
__thread_select_next::
        call    __thread_cleanup_terminated
        ld      hl, (_thread_first_waiting)
.waiting:
        ld      a, h
        or      l
        jr      z, .select
        push    hl
        pop     ix
        ld      e, 0(ix)
        ld      d, 1(ix)
        push    de
        ld      b, THREAD_NUM_EVENTS(ix)
        inc     b
        dec     b
        jr      z, .next
        ld      l, THREAD_WAIT(ix)
        ld      h, THREAD_WAIT+1(ix)
.event:
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     hl
        push    hl
        ex      de, hl
        ld      de, #5                 ; event state follows its object header
        add     hl, de
        ld      a, (hl)
        dec     a                      ; EVENT_SIGNALED = 1
        jr      nz, .not_signaled
        ld      (hl), a                 ; A=0: one signal wakes one waiter
.not_signaled:
        pop     hl
        jr      z, .wake
        djnz    .event
        jr      .next
.wake:
        ld      THREAD_STATE(ix), #STATE_RUNNING
        push    ix
        pop     de
        ld      hl, #_thread_first_waiting
        call    _list_remove
        ; DE is the removed thread. Insert through the null tail link using
        ; the ordinary list primitive, leaving existing runnable peers first.
        ; Head insertion lets two every-tick event waiters alternate forever:
        ; each wakes ahead of its peer and .select never reaches the UI.
        ld      hl, #_thread_first_running
.tail:
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        dec     hl
        ld      a, b
        or      c
        jr      z, .append
        ld      h, b
        ld      l, c
        jr      .tail
.append:
        call    _list_insert
.next:
        pop     hl
        jr      .waiting
.select:
        ld      hl, (_thread_current)
        ld      a, h
        or      l
        jr      z, .first
        push    hl
        pop     ix
        ld      a, THREAD_STATE(ix)
        dec     a                      ; STATE_RUNNING = 1
        jr      nz, .first
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      a, d
        or      e
        ret     nz
.first:
        ld      de, (_thread_first_running)
.done:
        ret
