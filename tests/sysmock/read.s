        ;; read.s  (sys backend: sim (in-RAM simulator))
        ;;
        ;; Thin entry wrapper around the shared simple-buffer file backend.
        ;; fd 0 reads from the synthetic console stream; fd >= 3 reads from
        ;; mounted in-memory files.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read
        .globl  __sys_none_read

        .area   _CODE

_read::
        jp      __sys_none_read
