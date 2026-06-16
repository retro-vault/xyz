        ;; cpm3_entry_dma_ptr_iy.s
        ;; Split from cpm3_find_open.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_entry_dma_ptr_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_entry_dma_ptr_iy

FD_OFF_DMA      .equ 53

        .area   _CODE
__cpm3_entry_dma_ptr_iy::
        push    de
        push    iy
        pop     hl
        ld      de,#FD_OFF_DMA
        add     hl,de
        pop     de
        ret

        ;; IY = current entry. Return carry if fpos < size, Z if equal.
