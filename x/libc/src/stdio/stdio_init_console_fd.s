        ;; stdio_init_console_fd.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_init_console_fd
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_init_console_fd
        .globl  __stdio_stream_accepts_output
        .globl  __stdio_set_count_zero

CTX_SINK_FD     .equ 2
CTX_SINK_KIND   .equ 0
CTX_SINK_TERM   .equ 1

        .area   _CODE
__stdio_init_console_fd::
        ld      CTX_SINK_FD(iy),a
        xor     a
        ld      CTX_SINK_KIND(iy),a
        ld      CTX_SINK_TERM(iy),a
        jp      __stdio_set_count_zero

__stdio_stream_accepts_output::
        ld      a,h
        or      l
        jr      z,__stdio_stream_accepts_output_fail
        ld      a,(hl)
        cp      #0x00
        jr      z,__stdio_stream_accepts_output_fail
        cp      #0xff
        jr      z,__stdio_stream_accepts_output_fail
        ret
__stdio_stream_accepts_output_fail:
        ld      hl,#0xFFFF
        or      a
        ret

