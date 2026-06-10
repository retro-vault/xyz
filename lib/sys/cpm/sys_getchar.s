        ;; sys_getchar.s  (sys backend: CP/M)
        ;;
        ;; Fetch one byte through the BDOS console-input call. CP/M echoes the
        ;; character as part of the BDOS service.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sys_getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_getchar
        .globl  ___sys_getchar

        .area   _CODE

__sys_getchar:
___sys_getchar::
        ld      c,#1
        call    5
        ld      e,a
        ld      d,#0x00
        ret
