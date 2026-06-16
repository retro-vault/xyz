        ;; cpm3_read_file.s  (sys backend: CP/M 3)
        ;;
        ;; FCB-backed read() path for file descriptors >= 3. Reached only
        ;; through __cpm3_read_file_vec, installed by _open.
        ;; Entry contract (set up by _read): HL = fd, DE = buf,
        ;; count at 4(ix), caller IX pushed, IX = frame. Returns DE = count
        ;; read or 0xFFFF, exiting with pop ix / ret.

        .module cpm3_read_file
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_read_file
        .globl  __cpm3_cmp_fpos_size_iy
        .globl  __cpm3_entry_dma_ptr_iy
        .globl  __cpm3_find_open
        .globl  __cpm3_get_current_user
        .globl  __cpm3_inc_fpos_iy
        .globl  __cpm3_loadbuf_read_iy
        .globl  __cpm3_set_user_a
        .globl  __cpm3_sync_iy
        .globl  __cpm3_tmp_len
        .globl  __cpm3_tmp_ptr
        .globl  __cpm3_tmp_saved_user

ACC_MASK        .equ 3
ACC_WRONLY      .equ 1
FD_OFF_BUFVALID .equ 4
FD_OFF_FLAGS    .equ 1
FD_OFF_FPOS0    .equ 5
FD_OFF_USER     .equ 2

        .area   _CODE
__cpm3_read_file::
        ld      (__cpm3_tmp_ptr),de     ; capture buf before BDOS clobbers DE
        call    __cpm3_find_open
        ld      a,h
        or      l
        jp      z,__cpm3_rf_fail
        push    hl
        pop     iy
        ld      a,FD_OFF_FLAGS(iy)
        and     #ACC_MASK
        cp      #ACC_WRONLY
        jp      z,__cpm3_rf_fail
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,FD_OFF_USER(iy)
        call    __cpm3_set_user_a
        call    __cpm3_sync_iy
        jp      nz,__cpm3_rf_restore_fail
        ld      c,4(ix)
        ld      b,5(ix)
        ld      (__cpm3_tmp_len),bc
__cpm3_read_file_loop:
        ld      a,b
        or      c
        jr      z,__cpm3_read_file_done
        call    __cpm3_cmp_fpos_size_iy
        jr      nc,__cpm3_read_file_done
        ld      a,FD_OFF_BUFVALID(iy)
        or      a
        jr      nz,__cpm3_read_have_buf
        call    __cpm3_loadbuf_read_iy
        jp      nz,__cpm3_rf_restore_fail
__cpm3_read_have_buf:
        ld      a,FD_OFF_FPOS0(iy)
        and     #0x7f
        ld      e,a
        ld      d,#0x00
        call    __cpm3_entry_dma_ptr_iy
        add     hl,de
        ld      a,(hl)
        ld      hl,(__cpm3_tmp_ptr)
        ld      (hl),a
        inc     hl
        ld      (__cpm3_tmp_ptr),hl
        call    __cpm3_inc_fpos_iy
        dec     bc
        ld      a,FD_OFF_FPOS0(iy)
        and     #0x7f
        jr      nz,__cpm3_read_file_loop
        xor     a
        ld      FD_OFF_BUFVALID(iy),a
        jr      __cpm3_read_file_loop
__cpm3_read_file_done:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        ld      hl,(__cpm3_tmp_len)
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
__cpm3_rf_restore_fail:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
__cpm3_rf_fail:
        ld      de,#0xffff
        pop     ix
        ret
