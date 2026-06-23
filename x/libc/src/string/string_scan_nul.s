        ;; string_scan_nul.s
        ;; Split from string_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module string_scan_nul
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_scan_nul

        .area   _CODE
__string_scan_nul::
        xor     a
        ld      bc,#0xffff
        cpir
        dec     hl
        ret

        ; __string_copy_cstr
        ; inputs: HL = source string, DE = destination string
        ; outputs: HL/DE advanced just past the copied NUL terminator
        ; clobbers: AF
