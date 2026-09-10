        ; Release an exited process and its owned resources.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module process_reap
        .optsdcc -mz80 sdcccall(1)

        .globl  _process_reap
        .globl  __heap
        .globl  __evt_first
        .globl  __tmr_first
        .globl  __svc_first
        .globl  _process_first
        .globl  _process_has_threads
        .globl  __process_find_owned
        .globl  _evt_destroy
        .globl  _tmr_uninstall
        .globl  _svc_unregister
        .globl  _mem_free_owner
        .globl  _so_destroy
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .equ    PROCESS_MAIN_THREAD,    13

        .area   _CODE

        ; _process_reap, sdcccall(1)
        ; input: hl = process
        ; output: none
        ; clobbers: af, bc, de, hl; preserves ix and iy
_process_reap::
        ld      a, h
        or      l
        ret     z
        push    ix
        ld      ix, #0
        add     ix, sp
        push    hl
        call    _enter_critical_section
        ld      l, -2(ix)
        ld      h, -1(ix)
        call    _process_has_threads
        or      a
        jr      nz, .reap_leave

        ld      l, -2(ix)
        ld      h, -1(ix)
        ld      de, #PROCESS_MAIN_THREAD
        add     hl, de
        xor     a
        ld      (hl), a
        inc     hl
        ld      (hl), a

.reap_events:
        ld      hl, (__evt_first)
        ld      e, -2(ix)
        ld      d, -1(ix)
        call    __process_find_owned
        ld      a, d
        or      e
        jr      z, .reap_timers
        ex      de, hl
        call    _evt_destroy
        jr      .reap_events

.reap_timers:
        ld      hl, (__tmr_first)
        ld      e, -2(ix)
        ld      d, -1(ix)
        call    __process_find_owned
        ld      a, d
        or      e
        jr      z, .reap_services
        ex      de, hl
        call    _tmr_uninstall
        jr      .reap_timers

.reap_services:
        ld      hl, (__svc_first)
        ld      e, -2(ix)
        ld      d, -1(ix)
        call    __process_find_owned
        ld      a, d
        or      e
        jr      z, .reap_memory
        ex      de, hl
        call    _svc_unregister
        jr      .reap_services

.reap_memory:
        ld      e, -2(ix)
        ld      d, -1(ix)
        ld      hl, #__heap
        call    _mem_free_owner
        ld      e, -2(ix)
        ld      d, -1(ix)
        ld      hl, #_process_first
        call    _so_destroy
.reap_leave:
        call    _leave_critical_section
        ld      sp, ix
        pop     ix
        ret
