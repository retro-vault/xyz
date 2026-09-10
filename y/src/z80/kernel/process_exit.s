        ; Exit the process represented by the current thread.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module process_exit
        .optsdcc -mz80 sdcccall(1)

        .globl  _process_exit
        .globl  _thread_current
        .globl  _thread_exit

        .area   _CODE

        ; Exit the process represented by the current thread.
_process_exit::
        ld      hl, (_thread_current)
        ld      a, h
        or      l
        ret     z
        jp      _thread_exit
