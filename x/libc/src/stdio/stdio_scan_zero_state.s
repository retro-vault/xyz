        ;; stdio_scan_zero_state.s
        ;; Split from scanf_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_scan_zero_state
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_scan_zero_state

SC_ASSIGNED_HI         .equ -90
SC_ASSIGNED_LO         .equ -91
SC_COUNT_HI            .equ -92
SC_COUNT_LO            .equ -93
SC_EOF                 .equ -106
SC_LENGTH              .equ -104
SC_SUPPRESS            .equ -105

        .area   _CODE
__stdio_scan_zero_state::
        xor     a
        ld      SC_EOF(ix),a
        ld      SC_SUPPRESS(ix),a
        ld      SC_LENGTH(ix),a
        ld      SC_COUNT_LO(ix),a
        ld      SC_COUNT_HI(ix),a
        ld      SC_ASSIGNED_LO(ix),a
        ld      SC_ASSIGNED_HI(ix),a
        ret

