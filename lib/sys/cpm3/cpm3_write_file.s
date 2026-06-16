        ;; cpm3_write_file.s  (sys backend: CP/M 3)
        ;;
        ;; FCB-backed write() path for file descriptors >= 3. Reached only
        ;; through __cpm3_write_file_vec, installed by _open.
        ;; Entry contract (set up by _write): HL = fd, DE = buf,
        ;; count at 4(ix), caller IX pushed, IX = frame. Returns DE = count
        ;; written or 0xFFFF, exiting with pop ix / ret.

        .module cpm3_write_file
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_write_file
        .globl  __cpm3_cmp_fpos_size_iy
        .globl  __cpm3_copy_size_to_fpos_iy
        .globl  __cpm3_entry_dma_ptr_iy
        .globl  __cpm3_find_open
        .globl  __cpm3_flush_iy
        .globl  __cpm3_get_current_user
        .globl  __cpm3_inc_fpos_iy
        .globl  __cpm3_loadbuf_read_iy
        .globl  __cpm3_prepare_record_iy
        .globl  __cpm3_set_user_a
        .globl  __cpm3_tmp_len
        .globl  __cpm3_tmp_ptr
        .globl  __cpm3_tmp_saved_user
        .globl  __cpm3_zero_bytes

ACC_MASK        .equ 3
APPEND_FLAG     .equ 0x80
DMA_SIZE        .equ 128
FD_OFF_BUFVALID .equ 4
FD_OFF_DIRTY    .equ 3
FD_OFF_FLAGS    .equ 1
FD_OFF_FPOS0    .equ 5
FD_OFF_FSIZE0   .equ 9
FD_OFF_USER     .equ 2

        .area   _CODE
__cpm3_update_size_if_needed_iy:
        call    __cpm3_cmp_fpos_size_iy
        jr      c,__cpm3_update_size_done
        ret     z
        ld      a,FD_OFF_FPOS0(iy)
        ld      FD_OFF_FSIZE0(iy),a
        ld      a,FD_OFF_FPOS0 + 1(iy)
        ld      FD_OFF_FSIZE0 + 1(iy),a
        ld      a,FD_OFF_FPOS0 + 2(iy)
        ld      FD_OFF_FSIZE0 + 2(iy),a
        ld      a,FD_OFF_FPOS0 + 3(iy)
        ld      FD_OFF_FSIZE0 + 3(iy),a
__cpm3_update_size_done:
        ret

        ;; IY = current entry. Zero the entry's DMA buffer.
__cpm3_zero_dma_iy:
        call    __cpm3_entry_dma_ptr_iy
        ld      b,#DMA_SIZE
        jp      __cpm3_zero_bytes

        ;; IY = current entry. Load the record under fpos for writing.
__cpm3_loadbuf_write_iy:
        call    __cpm3_cmp_fpos_size_iy
        jr      c,__cpm3_loadbuf_write_existing
        jr      z,__cpm3_loadbuf_write_equal
        call    __cpm3_prepare_record_iy
        call    __cpm3_zero_dma_iy
        ld      a,#1
        ld      FD_OFF_BUFVALID(iy),a
        xor     a
        ret
__cpm3_loadbuf_write_equal:
        ld      a,FD_OFF_FSIZE0(iy)
        and     #0x7f
        jr      nz,__cpm3_loadbuf_write_existing
        call    __cpm3_prepare_record_iy
        call    __cpm3_zero_dma_iy
        ld      a,#1
        ld      FD_OFF_BUFVALID(iy),a
        xor     a
        ret
__cpm3_loadbuf_write_existing:
        jp      __cpm3_loadbuf_read_iy

__cpm3_write_file::
        ld      (__cpm3_tmp_ptr),de     ; capture buf before BDOS clobbers DE
        call    __cpm3_find_open
        ld      a,h
        or      l
        jp      z,__cpm3_wf_fail
        push    hl
        pop     iy
        ld      a,FD_OFF_FLAGS(iy)
        and     #ACC_MASK
        jp      z,__cpm3_wf_fail
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,FD_OFF_USER(iy)
        call    __cpm3_set_user_a
        ld      a,FD_OFF_FLAGS(iy)
        and     #APPEND_FLAG
        call    nz,__cpm3_copy_size_to_fpos_iy
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (__cpm3_tmp_len),bc
__cpm3_write_file_loop:
        ld      a,b
        or      c
        jr      z,__cpm3_write_file_done
        ld      a,FD_OFF_BUFVALID(iy)
        or      a
        jr      nz,__cpm3_write_have_buf
        call    __cpm3_loadbuf_write_iy
        jp      nz,__cpm3_wf_restore_fail
__cpm3_write_have_buf:
        ld      hl,(__cpm3_tmp_ptr)
        ld      a,(hl)
        inc     hl
        ld      (__cpm3_tmp_ptr),hl
        push    af
        ld      a,FD_OFF_FPOS0(iy)
        and     #0x7f
        ld      e,a
        ld      d,#0x00
        call    __cpm3_entry_dma_ptr_iy
        add     hl,de
        pop     af
        ld      (hl),a
        ld      a,#1
        ld      FD_OFF_DIRTY(iy),a
        call    __cpm3_inc_fpos_iy
        call    __cpm3_update_size_if_needed_iy
        dec     bc
        ld      a,FD_OFF_FPOS0(iy)
        and     #0x7f
        jr      nz,__cpm3_write_file_loop
        call    __cpm3_flush_iy
        jp      nz,__cpm3_wf_restore_fail
        xor     a
        ld      FD_OFF_BUFVALID(iy),a
        jr      __cpm3_write_file_loop
__cpm3_write_file_done:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        ld      hl,(__cpm3_tmp_len)
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
__cpm3_wf_restore_fail:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
__cpm3_wf_fail:
        ld      de,#0xffff
        pop     ix
        ret
