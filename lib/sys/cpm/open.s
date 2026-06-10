        ;; open.s  (sys backend: cpm)
        ;;
        ;; CP/M file translation is not wired into the Unix-like fd layer yet.
        ;; File opens fail until that shim is added.

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open

        .area   _CODE

_open::
        ld      de,#0xffff
        ret
