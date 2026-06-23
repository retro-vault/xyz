        ;; read.s  (sys backend: none — template)
        ;;
        ;; int read(int fd, void *buf, unsigned len) — DISK block read.
        ;;   HL = fd, DE = buf, BC = len    returns DE = bytes read, 0=EOF, -1.
        ;;
        ;; Console input is getchar(); this handles file descriptors (>= 3).
        ;; The template has no filesystem, so it returns EOF.

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read

        .area   _CODE
_read::
        ;; TODO: read BC bytes from file fd (HL) into (DE); return count.
        ld      de,#0                   ; EOF: no filesystem
        ret
