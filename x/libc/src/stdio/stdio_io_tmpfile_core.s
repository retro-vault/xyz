        ;; stdio_io_tmpfile_core.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_tmpfile_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_tmpfile_core
        .globl  __stdio_io_alloc_stream
        .globl  __stdio_io_invalidate_stream
        .globl  __stdio_io_reset_stream
        .globl  __stdio_io_slot_to_tmp_flag
        .globl  __stdio_io_slot_to_tmp_name
        .globl  __stdio_io_stream_slot
        .globl  __stdio_io_tmp_clear
        .globl  _open
        .globl  _tmpnam

O_CREAT_HI      .equ 0x01
O_RDWR_V        .equ 0x0002
O_TRUNC_HI      .equ 0x02

        .area   _CODE
__stdio_io_tmpfile_core::
        call    __stdio_io_alloc_stream
        ld      a,h
        or      l
        ret     z
        push    hl
        call    __stdio_io_stream_slot
        jr      c,__stdio_io_tmpfile_fail_nobc
        ld      c,a
        push    bc
        call    __stdio_io_slot_to_tmp_name
        call    _tmpnam
        ld      a,#(O_CREAT_HI | O_TRUNC_HI)
        ld      d,a
        ld      e,#O_RDWR_V
        ld      bc,#0x0000
        push    bc
        call    _open
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_io_tmpfile_have_fd
        ld      a,e
        cp      #0xff
        jr      z,__stdio_io_tmpfile_fail
__stdio_io_tmpfile_have_fd:
        pop     hl
        pop     bc
        ld      a,e
        push    hl
        call    __stdio_io_reset_stream
        pop     hl
        push    hl
        ld      a,c
        call    __stdio_io_slot_to_tmp_flag
        ld      (hl),#1
        pop     hl
        push    hl
        pop     de
        ret
__stdio_io_tmpfile_fail:
        pop     hl
        pop     bc
        call    __stdio_io_tmp_clear
        call    __stdio_io_invalidate_stream
        jr      __stdio_io_tmpfile_fail_return
__stdio_io_tmpfile_fail_nobc:
        pop     hl
        call    __stdio_io_tmp_clear
        call    __stdio_io_invalidate_stream
__stdio_io_tmpfile_fail_return:
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

        ;; freopen(path, mode, stream): close the current descriptor, clear any
        ;; tmpfile cleanup state, then reopen the supplied stream object in-place.
