        ;; feof.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module feof
        .optsdcc -mz80 sdcccall(1)

        .globl  _feof
        .globl  __stdio_io_require_stream

FILE_FLAG_EOF   .equ 0x01
FILE_OFF_FLAGS  .equ 1

        .area   _CODE
_feof::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_flag_false
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #FILE_FLAG_EOF
        jr      z,__stdio_io_flag_false
        ld      hl,#0x0001
        push    hl
        pop     de
        ret
__stdio_io_flag_false:
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

