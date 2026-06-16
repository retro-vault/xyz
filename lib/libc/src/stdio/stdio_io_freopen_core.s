        ;; stdio_io_freopen_core.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_freopen_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_freopen_core
        .globl  __stdio_io_invalidate_stream
        .globl  __stdio_io_parse_mode
        .globl  __stdio_io_require_stream
        .globl  __stdio_io_reset_stream
        .globl  __stdio_io_tmp_cleanup
        .globl  __stdio_io_tmp_clear
        .globl  _close
        .globl  _open

FILE_FREE_FD    .equ 0xff

        .area   _CODE
__stdio_io_freopen_core::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        push    de
        pop     hl
        call    __stdio_io_parse_mode
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_freopen_fail
        push    de
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_freopen_fail_flags
        push    hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_freopen_skip_close
        ld      l,a
        ld      h,#0x00
        call    _close
        pop     hl
        call    __stdio_io_tmp_cleanup
        jr      __stdio_io_freopen_open
__stdio_io_freopen_skip_close:
        pop     hl
        call    __stdio_io_tmp_clear
__stdio_io_freopen_open:
        pop     de
        pop     hl
        ld      bc,#0x0000
        push    bc
        call    _open
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_freopen_gotfd
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_freopen_open_fail
__stdio_io_freopen_gotfd:
        ld      l,4(ix)
        ld      h,5(ix)
        ld      a,e
        push    hl
        call    __stdio_io_reset_stream
        pop     hl
        push    hl
        pop     de
        pop     ix
        ret
__stdio_io_freopen_open_fail:
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_io_invalidate_stream
        jr      __stdio_io_freopen_fail_done
__stdio_io_freopen_fail_flags:
        pop     bc
__stdio_io_freopen_fail:
        pop     bc
__stdio_io_freopen_fail_done:
        ld      hl,#0x0000
        push    hl
        pop     de
        pop     ix
        ret

