        ;; freopen.s
        ;;
        ;; Public freopen() entry point. The shared core closes the old
        ;; descriptor, clears any tmpfile cleanup metadata, and reopens the
        ;; supplied FILE object in place.

        .module freopen
        .optsdcc -mz80 sdcccall(1)

        .globl  _freopen
        .globl  __stdio_io_freopen_core

        .area   _CODE

_freopen::
        jp      __stdio_io_freopen_core
