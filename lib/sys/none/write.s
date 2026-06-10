        ;; write.s  (sys backend: none / bare metal)
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.
        ;; fd 1/2 write to the captured console stream; fd >= 3 writes into
        ;; mounted in-memory files.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module write
        .optsdcc -mz80 sdcccall(1)

        .globl  _write
        .globl  __sys_none_write

        .area   _CODE

_write::
        jp      __sys_none_write
