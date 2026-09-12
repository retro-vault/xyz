        ; Shared scheduler queue roots and current-thread pointer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _thread_state
        .optsdcc -mz80 sdcccall(1)

        .globl  _thread_current
        .globl  _thread_first_suspended
        .globl  _thread_first_running
        .globl  _thread_first_waiting
        .globl  _thread_first_terminated

        .area   _BSS
_thread_current::
        .ds     2
        ; Keep these four roots adjacent: process_has_threads scans them.
_thread_first_suspended::
        .ds     2
_thread_first_running::
        .ds     2
_thread_first_waiting::
        .ds     2
_thread_first_terminated::
        .ds     2
