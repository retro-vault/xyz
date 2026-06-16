        ;; cpm3_clear_entry_iy.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_clear_entry_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_clear_entry_iy
        .globl  __cpm3_zero_bytes

FD_SIZE         .equ 181

        .area   _CODE
__cpm3_clear_entry_iy::
        push    iy
        pop     hl
        ld      b,#FD_SIZE
        jp      __cpm3_zero_bytes

        ;; IY = current entry. Return HL = entry FCB.
