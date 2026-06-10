        ;; close.s  (sys backend: none / bare metal)
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close
        .globl  __sys_none_close

        .area   _CODE

_close::
        jp      __sys_none_close
