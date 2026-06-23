        ;; string_compare_result.s
        ;; Split from string_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module string_compare_result
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_compare_result

        .area   _CODE
__string_compare_result::
        ld      de,#0xffff
        jr      c,__string_compare_result_done
        ld      de,#0x0000
        ret     z
        inc     e
__string_compare_result_done:
        ret

        ; __string_scan_nul
        ; inputs: HL = start of a NUL-terminated string
        ; outputs: HL = address of the terminating NUL byte
        ; clobbers: AF, BC
        ; notes:
        ;   CPIR stops one byte past the match, so HL is adjusted back.
