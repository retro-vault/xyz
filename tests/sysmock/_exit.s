        ;; _exit.s  (sys backend: sim (in-RAM simulator))
        ;;
        ;; Process-exit hook for bare metal. There is nowhere to return to, so
        ;; the stub simply halts forever after recording the last requested
        ;; status code for debugger inspection.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih



        .module _exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __exit
        .globl  __sys_exit_status

        .area   _CODE
__exit::
        ld      (__sys_exit_status),hl
sys_exit_halt:
        halt
        jr      sys_exit_halt
