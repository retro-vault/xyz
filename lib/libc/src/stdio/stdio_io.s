        ;; stdio_io.s
        ;;
        ;; Small fd-backed stdio helpers layered on top of Unix-style read/write
        ;; calls. The current FILE objects are tiny descriptors:
        ;;   +0  fd byte
        ;;   +1  flags (bit0 EOF, bit1 ERR)
        ;;   +2  pushback-valid
        ;;   +3  pushback-char
        ;;
        ;; This is intentionally unbuffered. It gives the libc a real input,
        ;; block-I/O, and basic file-open/seek surface on top of open/read/
        ;; write/lseek/close.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih








        .module stdio_io
        .optsdcc -mz80 sdcccall(1)

        ;; getchar() is provided directly by the platform console hook
        ;; (lib/sys/<backend>/getchar.s); getc(stdin)/scanf reach it through
        ;; the console branch of __stdio_io_getc_core.

        .area   _CONST
__stdio_io_tmpfile_mode:
        .ascii  "w+b\0"

