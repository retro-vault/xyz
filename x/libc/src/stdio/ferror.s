        ;; ferror.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ferror
        .optsdcc -mz80 sdcccall(1)

        .globl  _ferror
        .globl  __stdio_io_require_stream

FILE_FLAG_ERR   .equ 0x02
FILE_OFF_FLAGS  .equ 1

        .area   _CODE
_ferror::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_err_false
        ld      de,#FILE_OFF_FLAGS
        add     hl,de
        ld      a,(hl)
        and     #FILE_FLAG_ERR
        jr      z,__stdio_io_err_false
        ld      hl,#0x0001
        push    hl
        pop     de
        ret
__stdio_io_err_false:
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

