        ;; write.s  (sys backend: none — template)
        ;;
        ;; int write(int fd, const void *buf, unsigned len) — DISK block write.
        ;;   HL = fd, DE = buf, BC = len    returns DE = bytes written, or -1.
        ;;
        ;; Console output is putchar(); this handles file descriptors (>= 3).
        ;; The template has no filesystem, so it fails.

        .module write
        .optsdcc -mz80 sdcccall(1)

        .globl  _write

        .area   _CODE
_write::
        ;; TODO: write BC bytes from (DE) to file fd (HL); return count or -1.
        ld      de,#0xffff              ; -1: no filesystem
        ret
