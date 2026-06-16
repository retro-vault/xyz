        ;; cpm3_entry_fcb_ptr_iy.s
        ;; Split from cpm3_fd_slot_ptr.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_entry_fcb_ptr_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_entry_fcb_ptr_iy

FD_OFF_FCB      .equ 17

        .area   _CODE
__cpm3_entry_fcb_ptr_iy::
        push    de
        push    iy
        pop     hl
        ld      de,#FD_OFF_FCB
        add     hl,de
        pop     de
        ret

        ;; IY = current entry. Return HL = entry DMA buffer.
