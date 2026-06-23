        ; signal_common.s
        ;
        ; Shared signal disposition table for the xcc Z80 libc.  One entry per
        ; ISO C signal (SIGABRT..SIGTERM == 1..6); index 0 is unused.  Each
        ; slot is a 2-byte handler pointer, initialised to SIG_DFL (0).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module signal_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __signal_handlers

        .area   _DATA
__signal_handlers::
        .dw     0, 0, 0, 0, 0, 0, 0     ; [0..6], SIG_DFL
