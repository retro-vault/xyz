        ;; lseek.s  (sys backend: none — template)
        ;;
        ;; long lseek(int fd, long offset, int whence)
        ;;   HL = fd, DE:BC = offset, (stack) = whence   (sdcccall(1))
        ;;   returns DEHL = new absolute offset, or 0xFFFFFFFF (-1) on error.
        ;;
        ;; This template has no seekable objects, so lseek always fails.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek

        .area   _CODE
_lseek::
        ld      hl,#0xffff              ; -1 (low word)
        ld      de,#0xffff              ; -1 (high word)
        ret
