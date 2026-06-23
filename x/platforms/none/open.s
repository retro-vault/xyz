        ;; open.s  (sys backend: none — template)
        ;;
        ;; int open(const char *path, int flags, int mode)
        ;;   HL = path, DE = flags, BC = mode      (sdcccall(1))
        ;;   returns DE = file descriptor (>= 3), or 0xFFFF (-1) on error.
        ;;
        ;; This template has no filesystem, so open always fails.  To add files,
        ;; allocate a descriptor, open the underlying object, and return the fd;
        ;; read()/write()/lseek()/close() must then honour it.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open

        .area   _CODE
_open::
        ld      de,#0xffff              ; -1: no filesystem
        ret
