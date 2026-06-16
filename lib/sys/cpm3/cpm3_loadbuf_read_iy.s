        ;; cpm3_loadbuf_read_iy.s
        ;; Split from cpm3_cmp_fpos_size_iy.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cpm3_loadbuf_read_iy
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_loadbuf_read_iy
        .globl  __cpm3_prepare_record_iy
        .globl  __cpm3_entry_dma_ptr_iy
        .globl  __cpm3_entry_fcb_ptr_iy

BDOS            .equ 5
BDOS_SUCCESS    .equ 0
FD_OFF_BUFVALID .equ 4
F_DMAOFF        .equ 26
F_READRAND      .equ 33

        .area   _CODE
__cpm3_loadbuf_read_iy::
        push    bc
        push    de
        push    hl
        call    __cpm3_loadbuf_read_iy_impl
        pop     hl
        pop     de
        pop     bc
        ret
__cpm3_loadbuf_read_iy_impl:
        call    __cpm3_prepare_record_iy
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
        ld      c,#F_READRAND
        call    BDOS
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_loadbuf_read_fail
        ld      a,#1
        ld      FD_OFF_BUFVALID(iy),a
        xor     a
        ret
__cpm3_loadbuf_read_fail:
        ld      a,#1
        ret

        ;; IY = current entry. Ensure DMA holds the record addressed by fpos for
        ;; writing, preserving existing bytes when writing inside the current EOF.
        ;; Returns A = 0 on success.
