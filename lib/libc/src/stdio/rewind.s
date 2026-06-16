        ;; rewind.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module rewind
        .optsdcc -mz80 sdcccall(1)

        .globl  _rewind
        .globl  __stdio_io_require_stream
        .globl  __stdio_io_reset_stream
        .globl  __stdio_io_set_flags
        .globl  _lseek

FILE_FLAG_ERR   .equ 0x02
FILE_FREE_FD    .equ 0xff
SEEK_SET_V      .equ 0x0000

        .area   _CODE
_rewind::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_rewind_done
        push    hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_rewind_err
        ld      l,a
        ld      h,#0x00
        ld      bc,#SEEK_SET_V
        push    bc
        ld      bc,#0x0000
        push    bc
        push    bc
        call    _lseek
        pop     bc
        pop     bc
        pop     bc
        ld      a,h
        cp      #0xff
        jr      nz,__stdio_io_rewind_ok
        ld      a,l
        cp      #0xff
        jr      nz,__stdio_io_rewind_ok
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_rewind_ok
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_rewind_err
__stdio_io_rewind_ok:
        pop     hl
        ld      a,(hl)
        call    __stdio_io_reset_stream
        jr      __stdio_io_rewind_done
__stdio_io_rewind_err:
        pop     hl
        ld      a,#FILE_FLAG_ERR
        call    __stdio_io_set_flags
__stdio_io_rewind_done:
        ret
