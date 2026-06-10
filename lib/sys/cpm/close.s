        ;; close.s  (sys backend: cpm)
        ;;
        ;; CP/M file translation is not wired into the Unix-like fd layer yet.
        ;; File closes fail until that shim is added.

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close

        .area   _CODE

_close::
        ld      de,#0xffff
        ret
