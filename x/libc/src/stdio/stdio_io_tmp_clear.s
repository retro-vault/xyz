        ;; stdio_io_tmp_clear.s
        ;; Split from stdio_io.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_io_tmp_clear
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_io_tmp_clear
        .globl  __stdio_io_slot_to_tmp_flag
        .globl  __stdio_io_stream_slot

        .area   _CODE
__stdio_io_tmp_clear::
        push    hl
        call    __stdio_io_stream_slot
        jr      c,__stdio_io_tmp_clear_done
        call    __stdio_io_slot_to_tmp_flag
        xor     a
        ld      (hl),a
__stdio_io_tmp_clear_done:
        pop     hl
        ret

        ;; HL = FILE*. If this is a tmpfile stream, unlink its slot-local name
        ;; after the descriptor has been closed and clear the tmp flag.
