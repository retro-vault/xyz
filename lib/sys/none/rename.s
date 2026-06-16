        ;; rename.s  (sys backend: none — template)
        ;;
        ;; int _rename(const char *oldpath, const char *newpath)
        ;;   HL = oldpath, DE = newpath         (sdcccall(1))
        ;;   returns DE = 0 on success, 0xFFFF (-1) on error.
        ;;
        ;; No filesystem: always fails.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename

        .area   _CODE
_rename::
        ld      de,#0xffff              ; -1: unsupported
        ret
