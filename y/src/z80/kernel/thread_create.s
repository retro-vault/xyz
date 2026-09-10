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

        ; _thread_create, sdcccall(1)
        ; inputs: hl = entry, de = stack size, process at sp+2
        ; outputs: de = thread or zero; removes process argument
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; frame: entry -2, stack size -4, thread -6, process +4
_thread_create::
        push    ix
        ld      ix, #0
        add     ix, sp
        push    hl
        push    de
        call    _enter_critical_section

        ld      hl, #0
        push    hl
        ld      de, #THREAD_SIZE
        ld      hl, #_thread_first_suspended
        call    _so_create
        push    de
        ld      a, d
        or      e
        jp      z, .create_finish

        push    de                      ; allocation owner = thread
        ld      e, -4(ix)
        ld      d, -3(ix)
        ld      hl, #__heap
        call    _mem_allocate
        ld      a, d
        or      e
        jr      nz, .create_stack_ready

        ld      e, -6(ix)
        ld      d, -5(ix)
        ld      hl, #_thread_first_suspended
        call    _so_destroy
        xor     a
        ld      d, a
        ld      e, a
        ld      -6(ix), a
        ld      -5(ix), a
        jr      .create_finish

.create_stack_ready:
        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      bc, #THREAD_WAIT
        add     hl, bc
        xor     a
        ld      (hl), a                 ; wait = NULL
        inc     hl
        ld      (hl), a

        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      bc, #THREAD_STATE
        add     hl, bc
        ld      (hl), #STATE_SUSPENDED

        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      bc, #THREAD_PROCESS
        add     hl, bc
        ld      a, 4(ix)
        ld      (hl), a
        inc     hl
        ld      a, 5(ix)
        ld      (hl), a

        ; sp = stack + stack_size - CONTEXT_SIZE
        ld      l, e
        ld      h, d
        ld      e, -4(ix)
        ld      d, -3(ix)
        add     hl, de
        ld      de, #-CONTEXT_SIZE
        add     hl, de
        ex      de, hl
        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      bc, #THREAD_SP
        add     hl, bc
        ld      (hl), e
        inc     hl
        ld      (hl), d

        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      e, -2(ix)
        ld      d, -1(ix)
        call    _thread_prepare_startup
        ld      e, -6(ix)
        ld      d, -5(ix)

.create_finish:
        call    _leave_critical_section
        ld      e, -6(ix)
        ld      d, -5(ix)
        ld      sp, ix
        pop     ix
        pop     hl                      ; return address
        pop     bc                      ; process argument
        push    hl
        ret
