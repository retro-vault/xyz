        ; Reclaim an exited process or an unreferenced library.
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
        .globl  __library_private_services
        .globl  __library_refs
        .globl  _process_first
        .globl  _process_has_threads
        .globl  __process_find_owned
        .globl  __so_reap
        .globl  _mem_free_owner
        .globl  _so_destroy
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .equ    PROCESS_FLAGS,     4
        .equ    LIBRARY_REFS,     13
        .equ    REFERENCE_LIBRARY, 4
        .area   _CODE

        ; inputs: hl = process/library; outputs: none
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; IX holds the owner. Libraries use main_thread as a refcount.
_process_reap::
        ld      a, h
        or      l
        ret     z
        push    ix
        push    hl
        pop     ix
        call    _enter_critical_section
        call    _process_has_threads    ; hl still the process/library pointer;
        or      a                       ; a library never owns a thread, so
        jr      nz, .done               ; this is also the library's own check
        bit     0, PROCESS_FLAGS(ix)
        jr      z, .resources
        ld      a, LIBRARY_REFS(ix)
        or      LIBRARY_REFS+1(ix)
        jr      nz, .done
.resources:
        ld      hl, #__evt_first
        call    .owned
        ld      hl, #__tmr_first
        call    .owned
        ld      hl, #__svc_first
        call    .owned
        ld      hl, #__library_private_services
        call    .owned
.references:
        ld      hl, (__library_refs)
        push    ix
        pop     de
        call    __process_find_owned
        ld      a, d
        or      e
        jr      z, .memory
        ld      hl, #REFERENCE_LIBRARY
        add     hl, de
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        push    bc
        ld      hl, #__library_refs
        call    _so_destroy
        pop     de
        ld      hl, #LIBRARY_REFS
        add     hl, de
        ld      a, (hl)
        dec     (hl)
        or      a
        jr      nz, .release
        inc     hl
        dec     (hl)
.release:
        ex      de, hl
        call    _process_reap
        jr      .references
.memory:
        push    ix
        pop     de
        ld      hl, #__heap
        call    _mem_free_owner
        push    ix
        pop     de
        ld      hl, #_process_first
        call    _so_destroy
.done:
        call    _leave_critical_section
        pop     ix
        ret
.owned:
        push    ix
        pop     de
        jp      __so_reap
