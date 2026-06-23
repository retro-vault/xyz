        ;; fclose.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fclose
        .optsdcc -mz80 sdcccall(1)

        .globl  _fclose
        .globl  __stdio_io_invalidate_stream
        .globl  __stdio_io_require_stream
        .globl  __stdio_io_tmp_cleanup
        .globl  _close

FILE_FREE_FD    .equ 0xff

        .area   _CODE
_fclose::
        call    __stdio_io_require_stream
        ld      a,h
        cp      #0xff
        jr      z,__stdio_io_fclose_fail
        push    hl
        ld      a,(hl)
        cp      #FILE_FREE_FD
        jr      z,__stdio_io_fclose_fail_pop
        cp      #3
        jr      c,__stdio_io_fclose_fail_pop
        ld      l,a
        ld      h,#0x00
        call    _close
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_fclose_ok
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_fclose_fail_pop
__stdio_io_fclose_ok:
        pop     hl
        call    __stdio_io_tmp_cleanup
        call    __stdio_io_invalidate_stream
        ld      hl,#0x0000
        push    hl
        pop     de
        ret
__stdio_io_fclose_fail_pop:
        pop     hl
__stdio_io_fclose_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        ret

        ;; tmpfile(): allocate a pooled FILE slot, generate a slot-local name,
        ;; open it with w+b semantics, and remember that fclose() must unlink
        ;; the backing file after closing the descriptor.
