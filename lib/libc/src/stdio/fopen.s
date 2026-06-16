        ;; fopen.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fopen
        .optsdcc -mz80 sdcccall(1)

        .globl  _fopen
        .globl  __stdio_io_alloc_stream
        .globl  __stdio_io_parse_mode
        .globl  __stdio_io_reset_stream
        .globl  _close
        .globl  _open

        .area   _CODE
_fopen::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        push    de
        pop     hl
        call    __stdio_io_parse_mode
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fopen_fail_saved_path
        pop     hl
        ld      bc,#0x0000
        push    bc
        call    _open
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_fopen_gotfd
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_fopen_fail
__stdio_io_fopen_gotfd:
        push    de
        call    __stdio_io_alloc_stream
        pop     de
        ld      a,h
        or      l
        jr      nz,__stdio_io_fopen_have_slot
        ld      l,e
        ld      h,d
        call    _close
        jr      __stdio_io_fopen_fail
__stdio_io_fopen_have_slot:
        ld      a,e
        push    hl
        call    __stdio_io_reset_stream
        pop     hl
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_fopen_fail_saved_path:
        pop     bc
__stdio_io_fopen_fail:
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret

