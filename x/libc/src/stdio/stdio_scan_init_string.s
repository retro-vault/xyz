        ;; stdio_scan_init_string.s
        ;; Split from scanf_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_scan_init_string
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_scan_init_string
        .globl  __stdio_scan_zero_state

SCAN_KIND_STRING        .equ 2
SC_KIND                .equ -107
SC_STR_HI              .equ -100
SC_STR_LO              .equ -101

        .area   _CODE
__stdio_scan_init_string::
        ld      SC_STR_LO(ix),l
        ld      SC_STR_HI(ix),h
        ld      a,#SCAN_KIND_STRING
        ld      SC_KIND(ix),a
        jp      __stdio_scan_zero_state

        ;; A = input byte -> Z when the byte is one of the scanf whitespace
        ;; characters consumed by format whitespace and by most conversions.
