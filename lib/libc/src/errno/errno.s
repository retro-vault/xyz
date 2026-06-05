        ; errno.s
        ;
        ; Backing storage for libc errno on the xcc Z80 target.
        ; The current libc is single-threaded, so one global integer cell is
        ; sufficient.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module errno
        .optsdcc -mz80 sdcccall(1)

        .globl  __errno_value

        .area   _DATA

__errno_value:
        .dw     0
