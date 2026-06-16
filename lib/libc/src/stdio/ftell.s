        ;; ftell.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ftell
        .optsdcc -mz80 sdcccall(1)

        .globl  _ftell
        .globl  __stdio_io_require_stream
        .globl  __stdio_io_set_flags
        .globl  _lseek

FILE_FLAG_ERR   .equ 0x02
FILE_FREE_FD    .equ 0xff
FILE_OFF_PUSHV  .equ 2
SEEK_CUR_V      .equ 0x0001

        .area   _CODE
_ftell::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_ftell_fail
        ld      c,l
        ld      b,h
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_ftell_err
        ld      l,a
        ld      h,#0x00
        push    bc
        ld      bc,#SEEK_CUR_V
        push    bc
        ld      bc,#0x0000
        push    bc
        push    bc
        call    _lseek
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        ld      a,h
        cp      #0xff
        jr      nz,__stdio_io_ftell_swap
        ld      a,l
        cp      #0xff
        jr      nz,__stdio_io_ftell_swap
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_ftell_swap
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_ftell_err
__stdio_io_ftell_swap:
        push    hl
        push    de
        ld      h,b
        ld      l,c
        ld      de,#FILE_OFF_PUSHV
        add     hl,de
        ld      a,(hl)
        pop     de
        pop     hl
        or      a
        jr      z,__stdio_io_ftell_done
        dec     de
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_ftell_done
        dec     hl
__stdio_io_ftell_done:
        ret
__stdio_io_ftell_err:
        ld      h,b
        ld      l,c
        ld      a,#FILE_FLAG_ERR
        call    __stdio_io_set_flags
__stdio_io_ftell_fail:
        ld      hl,#0xffff
        ld      de,#0xffff
        ret

