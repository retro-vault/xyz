        ;; fileio.s  (sys backend: CP/M 3)
        ;;
        ;; Shared CP/M 3 FCB-backed file-descriptor backend for libc.
        ;; Public syscall veneers live in open/read/write/close/lseek/etc.;
        ;; this module owns the common descriptor table, temporary FCB state,
        ;; path parsing, BDOS helpers, and file read/write/close engines.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fileio
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_copy_bytes
        .globl  __cpm3_zero_bytes
        .globl  __cpm3_fd_slot_ptr
        .globl  __cpm3_tmp_fcb
        .globl  __cpm3_tmp_len
        .globl  __cpm3_tmp_ptr
        .globl  __cpm3_tmp_rec
        .globl  __cpm3_tmp_rec2
        .globl  __cpm3_tmp_saved_user
        .globl  __cpm3_tmp_user
        .globl  __cpm3_clear_entry_iy
        .globl  __cpm3_entry_dma_ptr_iy
        .globl  __cpm3_entry_fcb_ptr_iy
        .globl  __cpm3_find_open
        .globl  __cpm3_get_current_user
        .globl  __cpm3_set_user_a
        .globl  __cpm3_cmp_fpos_size_iy
        .globl  __cpm3_inc_fpos_iy
        .globl  __cpm3_copy_size_to_fpos_iy
        .globl  __cpm3_shift7_from_ptr
        .globl  __cpm3_zero_tmprec
        .globl  __cpm3_prepare_record_iy
        .globl  __cpm3_flush_iy
        .globl  __cpm3_loadbuf_read_iy
        .globl  __cpm3_sync_iy
        .globl  __cpm3_parse_path
        .globl  __cpm3_close_file
        .globl  __cpm3_read_file
        .globl  __cpm3_write_file

        .equ    ACC_MASK,3
        .equ    ACC_WRONLY,1
        .equ    APPEND_FLAG,0x80
        .equ    BDOS,5
        .equ    BDOS_SUCCESS,0
        .equ    DMA_SIZE,128
        .equ    DRV_GET,25
        .equ    FCB_OFF_F6ATTR,6
        .equ    FCB_OFF_RREC0,33
        .equ    FCB_OFF_RREC1,34
        .equ    FCB_OFF_RREC2,35
        .equ    FCB_OFF_SEQREQ,32
        .equ    FCB_SIZE,36
        .equ    FD_FILE_BASE,3
        .equ    FD_OFF_BUFREC0,13
        .equ    FD_OFF_BUFVALID,4
        .equ    FD_OFF_DIRTY,3
        .equ    FD_OFF_DMA,53
        .equ    FD_OFF_FCB,17
        .equ    FD_OFF_FLAGS,1
        .equ    FD_OFF_FPOS0,5
        .equ    FD_OFF_FSIZE0,9
        .equ    FD_OFF_USER,2
        .equ    FD_SIZE,181
        .equ    F_ATTRIB,30
        .equ    F_CLOSE,16
        .equ    F_DMAOFF,26
        .equ    F_READRAND,33
        .equ    F_TRUNCATE,99
        .equ    F_USERNUM,32
        .equ    F_WRITERAND,34
        .equ    OPEN_COUNT,16

        .area   _CODE
__cpm3_copy_tmprec_to_tmprec2:
        ld      hl,#__cpm3_tmp_rec
        ld      de,#__cpm3_tmp_rec2
        ld      b,#4
        jp      __cpm3_copy_bytes

__cpm3_bin2bcd_a:
        ld      e,a
        xor     a
__cpm3_bin2bcd_tens:
        ld      d,a
        ld      a,e
        cp      #10
        jr      c,__cpm3_bin2bcd_done
        sub     #10
        ld      e,a
        ld      a,d
        add     a,#0x10
        jr      __cpm3_bin2bcd_tens
__cpm3_bin2bcd_done:
        ld      a,d
        add     a,e
        ret

        .area   _CODE
__cpm3_copy_bytes::
        ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        djnz    __cpm3_copy_bytes
        ret

        ;; Upper-case ASCII in A.

        .area   _CODE
__cpm3_zero_bytes::
        xor     a
__cpm3_zero_bytes_loop:
        ld      (hl),a
        inc     hl
        djnz    __cpm3_zero_bytes_loop
        ret

        ;; HL = ptr, B = byte count. Fill with spaces.

        .area   _DATA
__cpm3_fd_table:
        .ds     OPEN_COUNT * FD_SIZE

        .area   _CODE
__cpm3_fd_slot_ptr::
        ld      hl,#__cpm3_fd_table
        or      a
        ret     z
        ld      b,a
__cpm3_fd_slot_ptr_loop:
        ld      de,#FD_SIZE
        add     hl,de
        djnz    __cpm3_fd_slot_ptr_loop
        ret

        ;; HL = fd. Return HL = entry or 0 on failure.

        .area   _DATA
__cpm3_tmp_fcb::
        .ds     FCB_SIZE

        .area   _DATA
__cpm3_tmp_len::
        .dw     0

        .area   _DATA
__cpm3_tmp_ptr::
        .dw     0

        .area   _DATA
__cpm3_tmp_rec::
        .db     0, 0, 0, 0

        .area   _DATA
__cpm3_tmp_rec2::
        .db     0, 0, 0, 0

        .area   _DATA
__cpm3_tmp_saved_user::
        .db     0

        .area   _DATA
__cpm3_tmp_user::
        .db     0

        .area   _CODE
__cpm3_clear_entry_iy::
        push    iy
        pop     hl
        ld      b,#FD_SIZE
        jp      __cpm3_zero_bytes

        ;; IY = current entry. Return HL = entry FCB.

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

        .area   _CODE
__cpm3_find_open::
        ld      a,h
        or      a
        jr      nz,__cpm3_find_open_fail
        ld      a,l
        sub     #FD_FILE_BASE
        jr      c,__cpm3_find_open_fail
        cp      #OPEN_COUNT
        jr      nc,__cpm3_find_open_fail
        call    __cpm3_fd_slot_ptr
        ld      a,(hl)
        or      a
        ret     nz
__cpm3_find_open_fail:
        ld      hl,#0x0000
        ret

        ;; Return A = free slot, HL = entry. Carry set on failure.

        .area   _CODE
__cpm3_get_current_user::
        push    ix
        push    iy
        push    bc
        push    de
        push    hl
        ld      c,#F_USERNUM
        ld      e,#0xff
        call    BDOS
        pop     hl
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret

        .area   _CODE
__cpm3_set_user_a::
        push    ix
        push    iy
        push    bc
        push    de
        push    hl
        ld      e,a
        ld      c,#F_USERNUM
        call    BDOS
        pop     hl
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ret

        .area   _CODE
__cpm3_cmp_fpos_size_iy::
        ld      a,FD_OFF_FPOS0 + 3(iy)
        cp      FD_OFF_FSIZE0 + 3(iy)
        jr      c,__cpm3_cmp_ret
        ret     nz
        ld      a,FD_OFF_FPOS0 + 2(iy)
        cp      FD_OFF_FSIZE0 + 2(iy)
        jr      c,__cpm3_cmp_ret
        ret     nz
        ld      a,FD_OFF_FPOS0 + 1(iy)
        cp      FD_OFF_FSIZE0 + 1(iy)
        jr      c,__cpm3_cmp_ret
        ret     nz
        ld      a,FD_OFF_FPOS0(iy)
        cp      FD_OFF_FSIZE0(iy)
__cpm3_cmp_ret:
        ret

        ;; IY = current entry. fpos = 0.

        .area   _CODE
__cpm3_inc_fpos_iy::
        ld      a,FD_OFF_FPOS0(iy)
        inc     a
        ld      FD_OFF_FPOS0(iy),a
        ret     nz
        ld      a,FD_OFF_FPOS0 + 1(iy)
        inc     a
        ld      FD_OFF_FPOS0 + 1(iy),a
        ret     nz
        ld      a,FD_OFF_FPOS0 + 2(iy)
        inc     a
        ld      FD_OFF_FPOS0 + 2(iy),a
        ret     nz
        ld      a,FD_OFF_FPOS0 + 3(iy)
        inc     a
        ld      FD_OFF_FPOS0 + 3(iy),a
        ret

        ;; HL = pointer to a 32-bit little-endian value. Return A = low 7-bit
        ;; remainder, and place value>>7 into __cpm3_tmp_rec.

        .area   _CODE
__cpm3_copy_size_to_fpos_iy::
        ld      a,FD_OFF_FSIZE0(iy)
        ld      FD_OFF_FPOS0(iy),a
        ld      a,FD_OFF_FSIZE0 + 1(iy)
        ld      FD_OFF_FPOS0 + 1(iy),a
        ld      a,FD_OFF_FSIZE0 + 2(iy)
        ld      FD_OFF_FPOS0 + 2(iy),a
        ld      a,FD_OFF_FSIZE0 + 3(iy)
        ld      FD_OFF_FPOS0 + 3(iy),a
        ret

        ;; IY = current entry. size = fpos when fpos grew past EOF.

        .area   _CODE
__cpm3_shift7_from_ptr::
        ld      de,#__cpm3_tmp_rec
        ld      b,#4
        call    __cpm3_copy_bytes
        ld      a,(__cpm3_tmp_rec)
        and     #0x7f
        push    af
        ld      b,#7
__cpm3_shift7_loop:
        ld      hl,#__cpm3_tmp_rec + 3
        srl     (hl)
        dec     hl
        rr      (hl)
        dec     hl
        rr      (hl)
        dec     hl
        rr      (hl)
        djnz    __cpm3_shift7_loop
        pop     af
        ret

        ;; Shift __cpm3_tmp_rec left by 7.

        .area   _CODE
__cpm3_zero_tmprec::
        ld      hl,#__cpm3_tmp_rec
        ld      b,#4
        jp      __cpm3_zero_bytes

        ;; Copy __cpm3_tmp_rec into __cpm3_tmp_rec2.

        .area   _CODE
__cpm3_prepare_record_iy::
        push    iy
        pop     hl
        ld      de,#FD_OFF_FPOS0
        add     hl,de
        call    __cpm3_shift7_from_ptr
        ld      a,(__cpm3_tmp_rec)
        ld      FD_OFF_BUFREC0(iy),a
        ld      FD_OFF_FCB + FCB_OFF_RREC0(iy),a
        ld      a,(__cpm3_tmp_rec + 1)
        ld      FD_OFF_BUFREC0 + 1(iy),a
        ld      FD_OFF_FCB + FCB_OFF_RREC1(iy),a
        ld      a,(__cpm3_tmp_rec + 2)
        ld      FD_OFF_BUFREC0 + 2(iy),a
        ld      FD_OFF_FCB + FCB_OFF_RREC2(iy),a
        ret

        ;; IY = current entry. Load the current file record into DMA for reading.
        ;; Returns A = 0 on success.

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

        .area   _CODE
__cpm3_inc_tmprec:
        ld      hl,#__cpm3_tmp_rec
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret     nz
        inc     hl
        ld      a,(hl)
        inc     a
        ld      (hl),a
        ret

        ;; Clear __cpm3_tmp_rec.
__cpm3_sync_iy::
        push    bc
        push    de
        push    hl
        call    __cpm3_sync_iy_impl
        pop     hl
        pop     de
        pop     bc
        ret
__cpm3_sync_iy_impl:
        call    __cpm3_flush_iy
        ret     nz
        ld      a,FD_OFF_FLAGS(iy)
        and     #ACC_MASK
        ret     z
        push    iy
        pop     hl
        ld      de,#FD_OFF_FSIZE0
        add     hl,de
        call    __cpm3_shift7_from_ptr
        ld      b,a                     ; B = last-record byte count
        or      a
        jr      z,__cpm3_sync_no_ceil
        call    __cpm3_inc_tmprec
__cpm3_sync_no_ceil:
        ld      a,(__cpm3_tmp_rec)
        ld      FD_OFF_FCB + FCB_OFF_RREC0(iy),a
        ld      a,(__cpm3_tmp_rec + 1)
        ld      FD_OFF_FCB + FCB_OFF_RREC1(iy),a
        ld      a,(__cpm3_tmp_rec + 2)
        ld      FD_OFF_FCB + FCB_OFF_RREC2(iy),a
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        push    bc                      ; keep remainder across BDOS
        ld      c,#F_TRUNCATE
        call    BDOS
        pop     bc
        pop     iy
        pop     ix
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_sync_truncate_unsupported
        ld      a,b
        or      a
        ret     z
        ld      a,b
        ld      FD_OFF_FCB + FCB_OFF_SEQREQ(iy),a
        ld      a,FD_OFF_FCB + FCB_OFF_F6ATTR(iy)
        or      #0x80
        ld      FD_OFF_FCB + FCB_OFF_F6ATTR(iy),a
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        ld      c,#F_ATTRIB
        call    BDOS
        pop     iy
        pop     ix
        push    af                      ; keep BDOS status across restore
        ld      a,FD_OFF_FCB + FCB_OFF_F6ATTR(iy)
        and     #0x7f
        ld      FD_OFF_FCB + FCB_OFF_F6ATTR(iy),a
        pop     af
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_sync_fail
        xor     a
        ret
__cpm3_sync_truncate_unsupported:
        ;; Hosts without BDOS 99 (e.g. CP/M 2.2-style emulators) keep the
        ;; record-rounded length; that is not a close failure.
        xor     a
        ret
__cpm3_sync_fail:
        ld      a,#1
        ret

        ;; A -> BCD in A.

        .area   _DATA
__cpm3_tmp_ptr2:
        .dw     0
        .area   _CODE
__cpm3_fill_spaces:
        ld      a,#' '
__cpm3_fill_spaces_loop:
        ld      (hl),a
        inc     hl
        djnz    __cpm3_fill_spaces_loop
        ret

        ;; HL = src, DE = dst, B = byte count.
__cpm3_upper_a:
        cp      #'a'
        jr      c,__cpm3_upper_a_done
        cp      #'z' + 1
        jr      nc,__cpm3_upper_a_done
        sub     #32
__cpm3_upper_a_done:
        ret

        ;; Validate A as a CP/M filename character. Carry set when accepted.
__cpm3_is_name_char:
        cp      #'0'
        jr      c,__cpm3_name_char_special
        cp      #'9' + 1
        scf
        jr      c,__cpm3_name_char_ret
        cp      #'A'
        jr      c,__cpm3_name_char_special
        cp      #'Z' + 1
        scf
        jr      c,__cpm3_name_char_ret
__cpm3_name_char_special:
        cp      #'_'
        jr      z,__cpm3_name_char_ok
        cp      #'$'
        jr      z,__cpm3_name_char_ok
        cp      #'#'
        jr      z,__cpm3_name_char_ok
        cp      #'@'
        jr      z,__cpm3_name_char_ok
        or      a
        ret
__cpm3_name_char_ok:
        scf
__cpm3_name_char_ret:
        ret

__cpm3_get_current_drive:
        push    ix
        push    iy
        ld      c,#DRV_GET
        ld      e,#0x00
        call    BDOS
        pop     iy
        pop     ix
        inc     a
        ret

        ;; HL = path, DE = destination FCB. Writes default drive/user plus
        ;; upper-cased 8.3 name. On success A = 0 and __cpm3_tmp_user holds
        ;; the selected user area.
__cpm3_parse_path::
        ld      a,h
        or      l
        jp      z,__cpm3_parse_fail
        ld      (__cpm3_tmp_ptr),hl
        ex      de,hl
        ld      (__cpm3_tmp_ptr2),hl
        ld      b,#FCB_SIZE
        call    __cpm3_zero_bytes
        ld      hl,(__cpm3_tmp_ptr2)
        inc     hl                      ; FCB name field at fcb+1
        ld      b,#8
        call    __cpm3_fill_spaces
        ld      b,#3
        call    __cpm3_fill_spaces      ; extension follows the name
        call    __cpm3_get_current_drive
        ld      hl,(__cpm3_tmp_ptr2)
        ld      (hl),a
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_user),a
        ld      hl,(__cpm3_tmp_ptr)
        ld      a,(hl)
        or      a
        jp      z,__cpm3_parse_fail
        call    __cpm3_upper_a
        ld      c,a
        inc     hl
        ld      a,(hl)
        cp      #':'
        jr      nz,__cpm3_parse_no_drive
        ld      a,c
        cp      #'A'
        jp      c,__cpm3_parse_fail
        cp      #'P' + 1
        jp      nc,__cpm3_parse_fail
        sub     #'A' - 1
        ld      de,(__cpm3_tmp_ptr2)
        ld      (de),a
        inc     hl
        jr      __cpm3_parse_name_start
__cpm3_parse_no_drive:
        dec     hl                      ; back to the first name char
__cpm3_parse_name_start:
        ld      de,(__cpm3_tmp_ptr2)
        inc     de
        ld      c,#0
__cpm3_parse_name_loop:
        ld      a,(hl)
        or      a
        jr      z,__cpm3_parse_name_done
        cp      #'.'
        jr      z,__cpm3_parse_ext
        cp      #'['
        jr      z,__cpm3_parse_user
        call    __cpm3_upper_a
        push    af
        call    __cpm3_is_name_char
        jp      nc,__cpm3_parse_fail_pop
        pop     af
        ld      a,c
        cp      #8
        jp      nc,__cpm3_parse_fail
        ld      a,(hl)
        call    __cpm3_upper_a
        ld      (de),a
        inc     de
        inc     hl
        inc     c
        jr      __cpm3_parse_name_loop
__cpm3_parse_name_done:
        ld      a,c
        or      a
        jp      z,__cpm3_parse_fail
        xor     a
        ret
__cpm3_parse_ext:
        ld      a,c
        or      a
        jp      z,__cpm3_parse_fail
        inc     hl
        ld      de,(__cpm3_tmp_ptr2)
        ld      a,e
        add     a,#9
        ld      e,a
        ld      a,d
        adc     a,#0
        ld      d,a
        ld      c,#0
__cpm3_parse_ext_loop:
        ld      a,(hl)
        or      a
        jr      z,__cpm3_parse_ext_done
        cp      #'['
        jr      z,__cpm3_parse_user
        call    __cpm3_upper_a
        push    af
        call    __cpm3_is_name_char
        jp      nc,__cpm3_parse_fail_pop
        pop     af
        ld      a,c
        cp      #3
        jp      nc,__cpm3_parse_fail
        ld      a,(hl)
        call    __cpm3_upper_a
        ld      (de),a
        inc     de
        inc     hl
        inc     c
        jr      __cpm3_parse_ext_loop
__cpm3_parse_ext_done:
        xor     a
        ret
__cpm3_parse_user:
        inc     hl
        ld      a,(hl)
        call    __cpm3_upper_a
        cp      #'G'
        jr      nz,__cpm3_parse_user_digit
        inc     hl
        ld      a,(hl)
        call    __cpm3_upper_a
__cpm3_parse_user_digit:
        cp      #'0'
        jp      c,__cpm3_parse_fail
        cp      #'9' + 1
        jp      nc,__cpm3_parse_fail
        sub     #'0'
        ld      b,a
        inc     hl
        ld      a,(hl)
        cp      #'0'
        jr      c,__cpm3_parse_user_tail
        cp      #'9' + 1
        jr      nc,__cpm3_parse_user_tail
        ld      a,b
        add     a,a
        ld      c,a
        add     a,a
        add     a,a
        add     a,c
        ld      b,a
        ld      a,(hl)
        sub     #'0'
        add     a,b
        ld      b,a
        inc     hl
__cpm3_parse_user_tail:
        ld      a,(hl)
        cp      #']'
        jp      nz,__cpm3_parse_fail
        ld      a,b
        cp      #16
        jp      nc,__cpm3_parse_fail
        ld      (__cpm3_tmp_user),a
        xor     a
        ret
__cpm3_parse_fail_pop:
        pop     af
__cpm3_parse_fail:
        ld      a,#1
        ret

        ;; IY = current entry. Clear the whole slot.

        .area   _CODE
__cpm3_close_file::
        call    __cpm3_find_open
        ld      a,h
        or      l
        jr      z,__cpm3_cf_fail
        push    hl
        pop     iy
        call    __cpm3_get_current_user
        ld      (__cpm3_tmp_saved_user),a
        ld      a,FD_OFF_USER(iy)
        call    __cpm3_set_user_a
        call    __cpm3_sync_iy
        jr      nz,__cpm3_cf_restore_fail
        call    __cpm3_entry_fcb_ptr_iy
        ex      de,hl
        push    ix
        push    iy
        ld      c,#F_CLOSE
        call    BDOS
        pop     iy
        pop     ix
        push    af
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
        pop     af
        cp      #BDOS_SUCCESS
        jr      nz,__cpm3_cf_fail
        call    __cpm3_clear_entry_iy
        ld      de,#0x0000
        ret
__cpm3_cf_restore_fail:
        ld      a,(__cpm3_tmp_saved_user)
        call    __cpm3_set_user_a
__cpm3_cf_fail:
        ld      de,#0xffff
        ret

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
