        ;; stdio_scan_init_stream.s
        ;; Split from scanf_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_scan_init_stream
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_scan_init_stream
        .globl  __stdio_scan_init_stream_shared
        .globl  __stdio_scan_zero_state

SCAN_KIND_STREAM        .equ 1
SC_KIND                .equ -107
SC_STREAM_HI           .equ -102
SC_STREAM_LO           .equ -103

        .area   _CODE
__stdio_scan_init_stream::
__stdio_scan_init_stream_shared::
        ld      SC_STREAM_LO(ix),l
        ld      SC_STREAM_HI(ix),h
        ld      a,#SCAN_KIND_STREAM
        ld      SC_KIND(ix),a
        jp      __stdio_scan_zero_state

