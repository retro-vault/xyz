        ;; sys_exit.s  (sys backend: none / bare metal)
        ;;
        ;; Process-exit hook for bare metal. There is nowhere to return to, so
        ;; the stub simply halts forever after recording the last requested
        ;; status code for debugger inspection.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sys_exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_exit
        .globl  ___sys_exit
        .globl  __sys_exit_status
        .globl  ___sys_exit_status

        .area   _DATA
__sys_exit_status:
___sys_exit_status::
        .dw     0

        .area   _CODE

__sys_exit:
___sys_exit::
        ld      (__sys_exit_status),hl
sys_exit_halt:
        halt
        jr      sys_exit_halt
