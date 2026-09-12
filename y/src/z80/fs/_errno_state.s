        ; Scheduler-virtualized errno cell for the public YOS table.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _errno_state
        .globl  __errno_value
        .area   _BSS
__errno_value::
        .ds     2
