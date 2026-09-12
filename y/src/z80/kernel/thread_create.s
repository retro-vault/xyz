        ; Create a suspended thread and initial context.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module thread_create
        .optsdcc -mz80 sdcccall(1)
        .globl  _thread_create
        .globl  __heap
        .globl  _thread_first_suspended
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  _so_create
        .globl  _so_destroy
        .globl  _mem_allocate
        .globl  _thread_prepare_startup
        .equ    THREAD_SIZE,    24
        .equ    THREAD_SP,       4
        .equ    THREAD_WAIT,    16
        .equ    THREAD_PROCESS, 22
        .equ    CONTEXT_SIZE,   22
        .area   _CODE

        ; inputs: hl = entry, de = stack size, process at sp+2
        ; outputs: de = thread or zero; removes process argument
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; IX holds the thread. Stack saves entry and size until allocated.
_thread_create::
        push    ix
        push    de
        push    hl
        call    _enter_critical_section
        ex      de, hl
        ld      bc, #CONTEXT_SIZE
        or      a
        sbc     hl, bc
        jr      c, .failed
        ld      hl, #0
        push    hl
        ld      de, #THREAD_SIZE
        ld      hl, #_thread_first_suspended
        call    _so_create
        ld      a, d
        or      e
        jr      z, .failed
        push    de
        pop     ix
        push    de
        ld      hl, #4
        add     hl, sp
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      hl, #__heap
        call    _mem_allocate
        ld      a, d
        or      e
        jr      z, .destroy
        pop     bc                     ; entry
        pop     hl                     ; stack size
        push    bc
        add     hl, de
        ld      de, #-CONTEXT_SIZE
        add     hl, de
        ld      THREAD_SP(ix), l
        ld      THREAD_SP+1(ix), h
        push    ix
        pop     hl
        ld      de, #THREAD_WAIT
        add     hl, de
        xor     a
        ld      b, #6
.clear:
        ld      (hl), a
        inc     hl
        djnz    .clear
        ld      hl, #6
        add     hl, sp
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      THREAD_PROCESS(ix), e
        ld      THREAD_PROCESS+1(ix), d
        pop     de
        push    ix
        pop     hl
        call    _thread_prepare_startup
        push    ix
        pop     de
        jr      .done
.destroy:
        push    ix
        pop     de
        ld      hl, #_thread_first_suspended
        call    _so_destroy
.failed:
        pop     hl
        pop     hl
        ld      de, #0
.done:
        call    _leave_critical_section
        pop     ix
        pop     hl
        pop     bc
        jp      (hl)
