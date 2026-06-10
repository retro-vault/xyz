        ;; open.s  (sys backend: none / bare metal)
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open
        .globl  __sys_none_open

        .area   _CODE

_open::
        jp      __sys_none_open
