        ;; stdio_io_alloc_stream.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_alloc_stream
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_alloc_stream
        .globl  __stdio_io_file_pool

FILE_FREE_FD    .equ 0xff
FILE_POOL_COUNT .equ 4

        .area   _CODE
__stdio_io_alloc_stream::
        ld      hl,#__stdio_io_file_pool
        ld      b,#FILE_POOL_COUNT
__stdio_io_alloc_stream_loop:
        ld      a,(hl)
        or      a
        ret     z
        cp      #FILE_FREE_FD
        ret     z
        ld      de,#4
        add     hl,de
        djnz    __stdio_io_alloc_stream_loop
        ld      hl,#0x0000
        ret

        ;; Parse fopen() mode text at HL. On success return DE = open flags
        ;; and HL = 0. On failure return HL = 0xFFFF.
