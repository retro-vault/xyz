        ;; sys_getchar.s  (sys backend: generic z80)
        ;;
        ;; Generic bare-metal builds have no standard input device, so the hook
        ;; reports EOF immediately.
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
        ld      de,#0xffff
        ret
