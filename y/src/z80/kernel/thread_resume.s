        ; Move a suspended thread to the runnable queue.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module thread_resume
        .optsdcc -mz80 sdcccall(1)

        .globl  _thread_resume
        .globl  __thread_lswitch
        .globl  _thread_first_running
        .globl  _thread_first_suspended

        .equ    THREAD_SIZE,           39
        .equ    THREAD_SP,              5
        .equ    THREAD_WAIT,           17
        .equ    THREAD_NUM_EVENTS,     19
        .equ    THREAD_STATE,          20
        .equ    THREAD_PROCESS,        23
        .equ    CONTEXT_SIZE,          22

        .equ    STATE_SUSPENDED,        0
        .equ    STATE_RUNNING,          1
        .equ    STATE_TERMINATED,       4
        .equ    EVENT_SIGNALED,         1

        .area   _CODE

        ; Move a suspended thread to the runnable queue.
        ; input: hl = thread
_thread_resume::
        ld      bc, #0                  ; immediate = false
        push    bc
        ld      a, #STATE_RUNNING
        push    af
        inc     sp                      ; retain only state byte
        push    hl
        ld      de, #_thread_first_running
        ld      hl, #_thread_first_suspended
        call    __thread_lswitch
        ret
