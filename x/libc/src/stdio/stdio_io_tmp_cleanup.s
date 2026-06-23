        ;; stdio_io_tmp_cleanup.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_tmp_cleanup
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_tmp_cleanup
        .globl  __stdio_io_slot_to_tmp_flag
        .globl  __stdio_io_slot_to_tmp_name
        .globl  __stdio_io_stream_slot
        .globl  _unlink

        .area   _CODE
__stdio_io_tmp_cleanup::
        push    hl
        call    __stdio_io_stream_slot
        jr      c,__stdio_io_tmp_cleanup_done
        ld      c,a
        call    __stdio_io_slot_to_tmp_flag
        ld      a,(hl)
        or      a
        jr      z,__stdio_io_tmp_cleanup_done
        xor     a
        ld      (hl),a
        ld      a,c
        call    __stdio_io_slot_to_tmp_name
        call    _unlink
__stdio_io_tmp_cleanup_done:
        pop     hl
        ret

        ;; Return HL = free FILE slot or 0.
