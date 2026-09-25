        ; Check all four scheduler queues for a process's threads.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module process_has_threads
        .optsdcc -mz80 sdcccall(1)
        .globl  _process_has_threads
        .globl  _thread_first_suspended
        .equ    THREAD_PROCESS, 23
        .area   _CODE

        ; inputs: hl = process
        ; outputs: a = 1 / NZ if found, zero / Z otherwise
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; The four queue roots are consecutive in _thread_state.s.
_process_has_threads::
        ex      de, hl
        ld      hl, #_thread_first_suspended
        ld      b, #4
.queue:
        ld      c, (hl)
        inc     hl
        ld      a, (hl)
        inc     hl
        push    hl
        push    bc
        ld      h, a
        ld      l, c
        call    .scan
        pop     bc
        pop     hl
        ret     nz
        djnz    .queue
        ret
.scan:
        ld      a, h
        or      l
        ret     z
        push    hl
        ld      bc, #THREAD_PROCESS
        add     hl, bc
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        or      a
        sbc     hl, de
        pop     hl
        jr      z, .found
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        jr      .scan
.found:
        ld      a, #1
        or      a
        ret
