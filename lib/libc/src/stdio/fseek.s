        ;; fseek.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _fseek
        .globl  __stdio_io_require_stream
        .globl  __stdio_io_reset_stream
        .globl  __stdio_io_set_flags
        .globl  _lseek

FILE_FLAG_ERR   .equ 0x02
FILE_FREE_FD    .equ 0xff

        .area   _CODE
_fseek::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fseek_fail
        push    hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_fseek_err
        ld      l,a
        ld      h,#0x00
        ld      c,8(ix)
        ld      b,9(ix)
        push    bc
        ld      c,6(ix)
        ld      b,7(ix)
        push    bc
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        call    _lseek
        pop     bc
        pop     bc
        pop     bc
        ld      a,h
        cp      #0xff
        jr      nz,__stdio_io_fseek_ok
        ld      a,l
        cp      #0xff
        jr      nz,__stdio_io_fseek_ok
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_fseek_ok
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_fseek_err
__stdio_io_fseek_ok:
        pop     hl
        ld      a,(hl)
        call    __stdio_io_reset_stream
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_fseek_err:
        pop     hl
        ld      a,#FILE_FLAG_ERR
        call    __stdio_io_set_flags
__stdio_io_fseek_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        pop     ix
        ret

