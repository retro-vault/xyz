        ;; stdio_io_set_flags.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_set_flags
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_set_flags

FILE_OFF_FLAGS  .equ 1

        .area   _CODE
__stdio_io_set_flags::
        push    af
        push    hl
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        pop     de
        pop     af
        or      (hl)
        ld      (hl),a
        ex      de,hl
        ret

        ;; HL = FILE*, A = fd byte. Reset flags/pushback and install the fd.
