        ;; system.s
        ;;
        ;; There is no command processor behind this target's libc surface.
        ;; Querying with NULL reports "not available", and executing a command
        ;; fails with -1.

        .module system
        .optsdcc -mz80 sdcccall(1)

        .globl  _system

        .area   _CODE

_system::
        ld      a,h
        or      l
        jr      z,system_none
        ld      de,#0xffff
        ret
system_none:
        ld      de,#0x0000
        ret
