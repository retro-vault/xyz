        ;; exit_core.s
        ;;
        ;; Hand-written process-termination helpers for the xcc Z80 libc.
        ;; The handler tables remain tiny and deterministic, and the final
        ;; platform hand-off goes through _exit so each backend chooses
        ;; how "process termination" maps to its machine.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih




        .module exit_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_exit_status

        .area   _DATA
__libc_exit_status::
        .dw     0
