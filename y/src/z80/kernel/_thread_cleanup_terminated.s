        ; Reclaim terminated threads without using their active stacks.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _thread_cleanup_terminated
        .optsdcc -mz80 sdcccall(1)
        .globl  __thread_cleanup_terminated
        .globl  __sys_heap
        .globl  _thread_current
        .globl  _thread_first_terminated
        .globl  _mem_free_owner
        .globl  _so_destroy
        .globl  _process_reap
        .equ    THREAD_PROCESS, 22
        .area   _CODE

        ; inputs: none; outputs: none
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; Stack saves the next thread and process across destruction.
__thread_cleanup_terminated::
        push    ix
        ld      hl, (_thread_first_terminated)
.loop:
        ld      a, h
        or      l
        jr      z, .done
        push    hl
        pop     ix
        ld      e, 0(ix)
        ld      d, 1(ix)
        push    de
        ld      de, (_thread_current)
        or      a
        sbc     hl, de
        jr      z, .next
        ld      l, THREAD_PROCESS(ix)
        ld      h, THREAD_PROCESS+1(ix)
        push    hl
        push    ix
        pop     de
        ld      hl, #__sys_heap
        call    _mem_free_owner
        push    ix
        pop     de
        ld      hl, #_thread_first_terminated
        call    _so_destroy
        pop     hl
        call    _process_reap
.next:
        pop     hl
        jr      .loop
.done:
        pop     ix
        ret
