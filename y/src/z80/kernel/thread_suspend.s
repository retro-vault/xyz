        ; Suspend a runnable thread and yield.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module thread_suspend
        .optsdcc -mz80 sdcccall(1)

        .globl  _thread_suspend
        .globl  __thread_lswitch
        .globl  _thread_first_running
        .globl  _thread_first_suspended

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

        ; Move a runnable thread to the suspended queue and yield.
        ; input: hl = thread
_thread_suspend::
        ld      bc, #1                  ; immediate = true
        push    bc
        xor     a                       ; STATE_SUSPENDED
        push    af
        inc     sp
        push    hl
        ld      de, #_thread_first_suspended
        ld      hl, #_thread_first_running
        call    __thread_lswitch
        ret
