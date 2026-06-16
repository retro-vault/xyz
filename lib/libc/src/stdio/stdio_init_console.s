        ;; stdio_init_console.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_init_console
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_init_console
        .globl  __stdio_set_count_zero

CTX_SINK_FD     .equ 2
CTX_SINK_KIND   .equ 0
CTX_SINK_TERM   .equ 1

        .area   _CODE
__stdio_init_console::
        xor     a
        ld      CTX_SINK_KIND(iy),a
        ld      CTX_SINK_TERM(iy),a
        ld      a,#1
        ld      CTX_SINK_FD(iy),a
        jp      __stdio_set_count_zero

