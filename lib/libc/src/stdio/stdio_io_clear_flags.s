        ;; stdio_io_clear_flags.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_clear_flags
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_clear_flags

FILE_FLAG_EOF   .equ 0x01
FILE_FLAG_ERR   .equ 0x02
FILE_OFF_FLAGS  .equ 1

        .area   _CODE
__stdio_io_clear_flags::
        push    hl
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #~(FILE_FLAG_EOF | FILE_FLAG_ERR)
        ld      (hl),a
        pop     hl
        ret

        ;; HL = FILE*. A = flag bits to OR in.
