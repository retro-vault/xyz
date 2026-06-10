        ;; lseek.s  (sys backend: cpm)
        ;;
        ;; CP/M file translation is not wired into the Unix-like fd layer yet.
        ;; Seeks fail until that shim is added.

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek

        .area   _CODE

_lseek::
        ld      de,#0xffff
        ld      hl,#0xffff
        ret
