        ;; tmpfile.s
        ;;
        ;; Public tmpfile() entry point. The heavy lifting lives in the shared
        ;; fd-backed stdio core so that the cleanup state stays next to the
        ;; pooled FILE slots it manages.

        .module tmpfile
        .optsdcc -mz80 sdcccall(1)

        .globl  _tmpfile
        .globl  __stdio_io_tmpfile_core

        .area   _CODE

_tmpfile::
        jp      __stdio_io_tmpfile_core
