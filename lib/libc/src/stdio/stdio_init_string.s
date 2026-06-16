        ;; stdio_init_string.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_init_string
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_init_string
        .globl  __stdio_set_count_zero
        .globl  __stdio_store_sink_ptr_hl

CTX_SINK_KIND   .equ 0
CTX_SINK_TERM   .equ 1
SINK_STRING     .equ 0x01

        .area   _CODE
__stdio_init_string::
        ld      a,#SINK_STRING
        ld      CTX_SINK_KIND(iy),a
        ld      a,#1
        ld      CTX_SINK_TERM(iy),a
        call    __stdio_store_sink_ptr_hl
        jp      __stdio_set_count_zero

