        ; Create a process and its initial thread.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module process_start
        .optsdcc -mz80 sdcccall(1)
        .globl  _process_start
        .globl  _process_first
        .globl  _so_create
        .globl  _so_destroy
        .globl  _thread_create
        .globl  _thread_resume
        .globl  __string_copy
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __bank_current
        .equ    PROCESS_SIZE,        16
        .equ    PROCESS_FLAGS,        4
        .equ    PROCESS_NAME,         5
        .equ    PROCESS_MAIN_THREAD, 13
        .equ    PROCESS_BANK,        15
        .area   _CODE

        ; inputs: hl = name, de = entry, stack size at sp+2
        ; outputs: de = process or zero; removes stack-size argument
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; IX holds the process. Stack saves name and entry until copied.
_process_start::
        push    ix
        push    de
        push    hl
        call    _enter_critical_section
        ld      hl, #0
        push    hl
        ld      de, #PROCESS_SIZE
        ld      hl, #_process_first
        call    _so_create
        pop     hl
        ld      a, d
        or      e
        jr      z, .failed
        push    de
        pop     ix
        ld      PROCESS_FLAGS(ix), #0
        ld      bc, #PROCESS_NAME
        ex      de, hl
        add     hl, bc
        ld      b, #7
        call    __string_copy
        ; Common-memory entries are unbanked (FFh); an entry in the upper
        ; window belongs to whichever logical bank the loader selected.
        ld      PROCESS_BANK(ix), #0xff
        ld      hl, #6
        add     hl, sp
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        bit     7, d
        jr      z, .entry_ready
        ld      a, (__bank_current)
        ld      PROCESS_BANK(ix), a
.entry_ready:
        pop     hl
        push    ix
        call    _thread_create
        ld      PROCESS_MAIN_THREAD(ix), e
        ld      PROCESS_MAIN_THREAD+1(ix), d
        ld      a, d
        or      e
        jr      z, .destroy
        ex      de, hl
        call    _thread_resume
        push    ix
        pop     de
        jr      .done
.destroy:
        push    ix
        pop     de
        ld      hl, #_process_first
        call    _so_destroy
        ld      de, #0
        jr      .done
.failed:
        pop     hl
.done:
        call    _leave_critical_section
        pop     ix
        pop     hl
        pop     bc
        jp      (hl)
