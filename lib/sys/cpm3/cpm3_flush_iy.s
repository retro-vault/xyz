        ;; cpm3_flush_iy.s
        ;; Split from cpm3_find_open.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_flush_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_flush_iy
        .globl  __cpm3_entry_dma_ptr_iy
        .globl  __cpm3_entry_fcb_ptr_iy

BDOS            .equ 5
BDOS_SUCCESS    .equ 0
FCB_OFF_RREC0   .equ 33
FCB_OFF_RREC1   .equ 34
FCB_OFF_RREC2   .equ 35
FD_OFF_BUFREC0  .equ 13
FD_OFF_DIRTY    .equ 3
FD_OFF_FCB      .equ 17
F_DMAOFF        .equ 26
F_WRITERAND     .equ 34

        .area   _CODE
__cpm3_flush_iy::
        push    bc
        push    de
        push    hl
        call    __cpm3_flush_iy_impl
        pop     hl
        pop     de
        pop     bc
        ret
__cpm3_flush_iy_impl:
        ld      a,FD_OFF_DIRTY(iy)
        or      a
        ret     z
        ld      a,FD_OFF_BUFREC0(iy)
        ld      FD_OFF_FCB + FCB_OFF_RREC0(iy),a
        ld      a,FD_OFF_BUFREC0 + 1(iy)
        ld      FD_OFF_FCB + FCB_OFF_RREC1(iy),a
        ld      a,FD_OFF_BUFREC0 + 2(iy)
        ld      FD_OFF_FCB + FCB_OFF_RREC2(iy),a
        call    __cpm3_entry_dma_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        ld      c,#F_DMAOFF
        call    BDOS
        pop     iy
        pop     ix
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        ld      c,#F_WRITERAND
        call    BDOS
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_flush_fail
        xor     a
        ld      FD_OFF_DIRTY(iy),a
        ret
__cpm3_flush_fail:
        ld      a,#1
        ret

        ;; IY = current entry. Zero the whole DMA block.
