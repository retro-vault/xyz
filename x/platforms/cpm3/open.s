        ;; open.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open
        .globl  __cpm3_write_file
        .globl  __cpm3_read_file
        .globl  __cpm3_close_file
        .globl  __cpm3_write_file_vec
        .globl  __cpm3_read_file_vec
        .globl  __cpm3_close_file_vec
        .globl  __cpm3_clear_entry_iy
        .globl  __cpm3_copy_bytes
        .globl  __cpm3_copy_size_to_fpos_iy
        .globl  __cpm3_entry_fcb_ptr_iy
        .globl  __cpm3_fd_slot_ptr
        .globl  __cpm3_get_current_user
        .globl  __cpm3_parse_path
        .globl  __cpm3_set_user_a
        .globl  __cpm3_tmp_fcb
        .globl  __cpm3_tmp_rec
        .globl  __cpm3_tmp_saved_user
        .globl  __cpm3_tmp_user
        .globl  __cpm3_zero_tmprec

        .equ    ACC_MASK,3
        .equ    APPEND_FLAG,0x80
        .equ    BDOS,5
        .equ    BDOS_SUCCESS,0
        .equ    DMA_SIZE,128
        .equ    FCB_OFF_RREC0,33
        .equ    FCB_OFF_RREC1,34
        .equ    FCB_OFF_RREC2,35
        .equ    FCB_OFF_SEQREQ,32
        .equ    FCB_SIZE,36
        .equ    FD_FILE_BASE,3
        .equ    FD_OFF_ACTIVE,0
        .equ    FD_OFF_BUFVALID,4
        .equ    FD_OFF_DIRTY,3
        .equ    FD_OFF_FLAGS,1
        .equ    FD_OFF_FPOS0,5
        .equ    FD_OFF_FSIZE0,9
        .equ    FD_OFF_USER,2
        .equ    F_DELETE,19
        .equ    F_MAKE,22
        .equ    F_OPEN,15
        .equ    F_SIZE,35
        .equ    OPEN_COUNT,16
        .equ    O_APPEND_HI,0x04
        .equ    O_CREAT_HI,0x01
        .equ    O_TRUNC_HI,0x02

        .area   _DATA
__cpm3_tmp_flags:
        .dw     0
__cpm3_tmp_slot:
        .db     0
__cpm3_open_path:
        .dw     0

        .area   _CODE
__cpm3_find_free:
        xor     a
        ld      b,#OPEN_COUNT
__cpm3_find_free_loop:
        push    af
        call    __cpm3_fd_slot_ptr
        ld      a,(hl)
        or      a
        jr      z,__cpm3_find_free_hit
        pop     af
        inc     a
        djnz    __cpm3_find_free_loop
        scf
        ret
__cpm3_find_free_hit:
        pop     af
        or      a
        ret

        ;; HL = ptr, B = byte count. Fill with zero.
__cpm3_zero_fpos_iy:
        xor     a
        ld      FD_OFF_FPOS0(iy),a
        ld      FD_OFF_FPOS0 + 1(iy),a
        ld      FD_OFF_FPOS0 + 2(iy),a
        ld      FD_OFF_FPOS0 + 3(iy),a
        ret

        ;; IY = current entry. fpos = size.
__cpm3_shiftl7_tmprec:
        ld      b,#7
__cpm3_shiftl7_loop:
        ld      hl,#__cpm3_tmp_rec
        sla     (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        djnz    __cpm3_shiftl7_loop
        ret

        ;; Decrement the 32-bit value in __cpm3_tmp_rec.
__cpm3_dec_tmprec:
        ld      hl,#__cpm3_tmp_rec
        ld      a,(hl)
        dec     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        dec     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        dec     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        dec     a
        ld      (hl),a
        ret

        ;; Increment the 32-bit value in __cpm3_tmp_rec.
__cpm3_tmprec_from_open_fcb_size:
        ld      a,(__cpm3_tmp_fcb + FCB_OFF_RREC0)
        ld      (__cpm3_tmp_rec),a
        ld      a,(__cpm3_tmp_fcb + FCB_OFF_RREC1)
        ld      (__cpm3_tmp_rec + 1),a
        ld      a,(__cpm3_tmp_fcb + FCB_OFF_RREC2)
        ld      (__cpm3_tmp_rec + 2),a
        xor     a
        ld      (__cpm3_tmp_rec + 3),a
        ld      a,(__cpm3_tmp_rec)
        ld      b,a
        ld      a,(__cpm3_tmp_rec + 1)
        or      b
        ld      b,a
        ld      a,(__cpm3_tmp_rec + 2)
        or      b
        jr      z,__cpm3_tmprec_from_open_fcb_done
        call    __cpm3_dec_tmprec
        call    __cpm3_shiftl7_tmprec
        ld      a,(__cpm3_tmp_fcb + FCB_OFF_SEQREQ)
        or      a
        jr      nz,__cpm3_tmprec_lrb_ready
        ld      a,#DMA_SIZE
__cpm3_tmprec_lrb_ready:
        ld      hl,#__cpm3_tmp_rec
        add     a,(hl)
        ld      (hl),a
        jr      nc,__cpm3_tmprec_from_open_fcb_done
        inc     hl
        inc     (hl)
        ret     nz
        inc     hl
        inc     (hl)
        ret     nz
        inc     hl
        inc     (hl)
        ret
__cpm3_tmprec_from_open_fcb_done:
        ret

        ;; IY = current entry. Return A = 0 on success after writing the DMA
        ;; buffer to the cached random record when dirty.
_open::
        push    ix
        ld      ix,#0
        add     ix,sp
        ;; First use of file I/O: install the FCB-backed handlers so
        ;; read/write/close can reach file descriptors.
        push    hl
        ld      hl,#__cpm3_write_file
        ld      (__cpm3_write_file_vec),hl
        ld      hl,#__cpm3_read_file
        ld      (__cpm3_read_file_vec),hl
        ld      hl,#__cpm3_close_file
        ld      (__cpm3_close_file_vec),hl
        pop     hl
        ld      a,h
        or      l
        jp      z,__cpm3_open_fail
        ld      (__cpm3_open_path),hl   ; find_free clobbers HL
        ld      (__cpm3_tmp_flags),de
        call    __cpm3_find_free
        jp      c,__cpm3_open_fail
        ld      (__cpm3_tmp_slot),a
        push    hl
        pop     iy
        call    __cpm3_clear_entry_iy
        ld      de,#__cpm3_tmp_fcb
        ld      hl,(__cpm3_open_path)
        call    __cpm3_parse_path
        jp      nz,__cpm3_open_fail
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,(__cpm3_tmp_user)
        call    __cpm3_set_user_a
        ld      a,(__cpm3_tmp_flags + 1)
        and     #O_TRUNC_HI
        jr      z,__cpm3_open_try_existing
        call    __cpm3_open_truncate_or_make
        jp      nz,__cpm3_open_restore_fail
        call    __cpm3_zero_tmprec
        jr      __cpm3_open_install
__cpm3_open_try_existing:
        ld      a,#0xff
        ld      (__cpm3_tmp_fcb + FCB_OFF_SEQREQ),a
        ld      de,#__cpm3_tmp_fcb
        push    ix
        push    iy
        ld      c,#F_OPEN
        call    BDOS
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        jr      z,__cpm3_open_existing_ok
        ld      a,(__cpm3_tmp_flags + 1)
        and     #O_CREAT_HI
        jp      z,__cpm3_open_restore_fail
        ld      de,#__cpm3_tmp_fcb
        push    ix
        push    iy
        ld      c,#F_MAKE
        call    BDOS
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        jp      nz,__cpm3_open_restore_fail
        call    __cpm3_zero_tmprec
        jr      __cpm3_open_install
__cpm3_open_existing_ok:
        ld      de,#__cpm3_tmp_fcb
        push    ix
        push    iy
        ld      c,#F_SIZE
        call    BDOS
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        jp      nz,__cpm3_open_restore_fail
        call    __cpm3_tmprec_from_open_fcb_size
__cpm3_open_install:
        ld      a,#1
        ld      FD_OFF_ACTIVE(iy),a
        ld      a,(__cpm3_tmp_user)
        ld      FD_OFF_USER(iy),a
        xor     a
        ld      FD_OFF_DIRTY(iy),a
        ld      FD_OFF_BUFVALID(iy),a
        ld      a,(__cpm3_tmp_flags)
        and     #ACC_MASK
        ld      b,a
        ld      a,(__cpm3_tmp_flags + 1)
        and     #O_APPEND_HI
        jr      z,__cpm3_open_flags_ready
        ld      a,b
        or      #APPEND_FLAG
        ld      b,a
__cpm3_open_flags_ready:
        ld      a,b
        ld      FD_OFF_FLAGS(iy),a
        call    __cpm3_zero_fpos_iy
        ld      a,(__cpm3_tmp_rec)
        ld      FD_OFF_FSIZE0(iy),a
        ld      a,(__cpm3_tmp_rec + 1)
        ld      FD_OFF_FSIZE0 + 1(iy),a
        ld      a,(__cpm3_tmp_rec + 2)
        ld      FD_OFF_FSIZE0 + 2(iy),a
        ld      a,(__cpm3_tmp_rec + 3)
        ld      FD_OFF_FSIZE0 + 3(iy),a
        ld      a,FD_OFF_FLAGS(iy)
        and     #APPEND_FLAG
        call    nz,__cpm3_copy_size_to_fpos_iy
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        ld      hl,#__cpm3_tmp_fcb
        ld      b,#FCB_SIZE
        call    __cpm3_copy_bytes
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        ld      a,(__cpm3_tmp_slot)
        add     a,#FD_FILE_BASE
        ld      e,a
        ld      d,#0x00
        pop     ix
        ret
__cpm3_open_restore_fail:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
__cpm3_open_fail:
        ld      de,#0xffff
        pop     ix
        ret

__cpm3_open_truncate_or_make:
        ld      de,#__cpm3_tmp_fcb
        push    ix
        push    iy
        ld      c,#F_DELETE
        call    BDOS
        pop     iy
        pop     ix
        ld      de,#__cpm3_tmp_fcb
        push    ix
        push    iy
        ld      c,#F_MAKE
        call    BDOS
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        ret

