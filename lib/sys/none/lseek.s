        ;; lseek.s  (sys backend: none / bare metal)
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek
        .globl  __sys_none_lseek

        .area   _CODE

_lseek::
        jp      __sys_none_lseek
