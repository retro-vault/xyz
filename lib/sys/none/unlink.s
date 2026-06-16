        ;; unlink.s  (sys backend: none — template)
        ;;
        ;; int _unlink(const char *path)   (backs remove()/unlink())
        ;;   HL = path                          (sdcccall(1))
        ;;   returns DE = 0 on success, 0xFFFF (-1) on error.
        ;;
        ;; No filesystem: always fails.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  _unlink

        .area   _CODE
_unlink::
        ld      de,#0xffff              ; -1: unsupported
        ret
