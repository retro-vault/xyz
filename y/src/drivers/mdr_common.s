        ;; mdr_common.s
        ;;
        ;; ZX Spectrum Microdrive driver: shared helpers/state.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_common

        .globl  _mdr_name_match10
        .globl  _mdr_make_name10
        .globl  _mdr_find_file_size
        .globl  _mdr_dir

        .equ    MD_DATA, 0xe7           ; microdrive data port
        .equ    MD_CTRL, 0xef           ; microdrive control/status port
        .equ    MD_SEL, 0xf7            ; drive select data port

        ;; port EFh read bits (Interface 1)
        .equ    MD_R_WP, 0x01           ; bit 0: write protect
        .equ    MD_R_SYNC, 0x10         ; bit 4: sync
        .equ    MD_R_GAP, 0x20          ; bit 5: gap
        .equ    MD_R_GAP_ANY, 0x24      ; accept either gap bit encoding
        .equ    MD_R_GAPSYNC, 0x30      ; gap+sync mask
        .equ    MD_R_SYNC_ALT, 0x02     ; alt sync bit seen in some IF1 docs/emus
        .equ    MD_R_GAP_ALT, 0x04      ; alt gap bit seen in some IF1 docs/emus
        .equ    MD_R_SYNC_ANY, 0x12     ; accept either sync bit encoding
        .equ    MD_R_GAPSYNC_ALT, 0x06  ; alt gap+sync mask

        .equ    MD_CTL_READ, 0xEE       ; IF1 read mode control value

        ;; port EFh write bits (Interface 1)
        .equ    MD_W_DATA, 0x01         ; bit 0: comms data (inverted on line)
        .equ    MD_W_CLK, 0x02          ; bit 1: comms clock
        .equ    MD_W_RW, 0x04           ; bit 2: 1=read, 0=write
        .equ    MD_W_ERASE, 0x08        ; bit 3: erase (active)

        ;; md_record_t field offsets within rec_buf
        .equ    REC_LEN, 2
        .equ    REC_FNAME, 4
        .equ    REC_CHK, 14             ; uint8_t checksum of bytes 0..13

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; __mdr_read_hdr_rec
        ;; Dispatch strategy:
        ;;   fetch and validate one header block, then one record block.
        ;;
        ;; Signature:
        ;;   carry = __mdr_read_hdr_rec(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_read_hdr_rec::
        ld	b,#8                    ; try up to 8 headers
.rhr_hdr_try:
        push	bc
        ld	hl,#header_buf
        call	__mdr_get_m_hd
        pop	bc
        jr	c,.rhr_fail
        ld	hl,#header_buf
        ld	b,#14
        call	__mdr_calc_checksum
        ld	c,a
        ld	a,(header_buf + 14)
        cp	c
        jr	nz,.rhr_hdr_next
        ld	a,(header_buf)
        bit	0,a                     ; must be header block
        jr	nz,.rhr_hdr_ok
.rhr_hdr_next:
        djnz	.rhr_hdr_try
        jr	.rhr_fail
.rhr_hdr_ok:
        ld	b,#8                    ; try up to 8 record descriptors
.rhr_rec_try:
        push	bc
        ld	hl,#rec_buf
        call	__mdr_get_m_hd
        pop	bc
        jr	c,.rhr_fail
        ld	hl,#rec_buf
        ld	b,#14
        call	__mdr_calc_checksum
        ld	c,a
        ld	a,(rec_buf + 14)
        cp	c
        jr	nz,.rhr_rec_next
        ld	a,(rec_buf)
        bit	0,a                     ; must be descriptor, not header
        jr	z,.rhr_ok
.rhr_rec_next:
        djnz	.rhr_rec_try
.rhr_fail:
        scf
        ret
.rhr_ok:
        or	a                       ; clear carry
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_rec_len_valid
        ;; Dispatch strategy:
        ;;   validate rec_buf length field is within 1..512.
        ;;
        ;; Signature:
        ;;   carry = __mdr_rec_len_valid(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_rec_len_valid::
        ld	a,(rec_buf + REC_LEN + 1) ; high byte
        cp	#2
        jr	c,.rlv_lochk
        jr	nz,.rlv_bad
        ld	a,(rec_buf + REC_LEN)    ; hi==2 => only 0x0200 is valid
        or	a
        jr	nz,.rlv_bad
        or	a                       ; valid, clear carry
        ret
.rlv_lochk:
        ld	a,(rec_buf + REC_LEN + 1)
        or	a
        jr	nz,.rlv_ok
        ld	a,(rec_buf + REC_LEN)    ; reject zero length
        or	a
        jr	z,.rlv_bad
.rlv_ok:
        or	a                       ; valid, clear carry
        ret
.rlv_bad:
        scf
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_name_match10
        ;; Dispatch strategy:
        ;;   compare 10-char record field with either padded or C-string name.
        ;;
        ;; Signature:
        ;;   carry = __mdr_name_match10(const char *rec10, const char *name)
        ;;
        ;; Arguments:
        ;;   HL = record filename field (10 bytes)
        ;;   DE = user filename pointer (C string or 10-char padded)
__mdr_name_match10::
        ld	c,#10
        ld	b,#0                    ; b=1 once trailing-space compare mode starts
.nm_cmp:
        ld	a,b
        or	a
        jr	nz,.nm_space
        ld	a,(de)
        or	a
        jr	nz,.nm_have
        ld	b,#1
.nm_space:
        ld	a,#' '
.nm_have:
        cp	(hl)
        jr	nz,.nm_miss
        inc	hl
        ld	a,b
        or	a
        jr	nz,.nm_noinc
        inc	de
.nm_noinc:
        dec	c
        jr	nz,.nm_cmp
        or	a                       ; clear carry
        ret
.nm_miss:
        scf
        ret

        ;; ------------------------------------------------------------
        ;; _mdr_name_match10
        ;; C-callable wrapper around __mdr_name_match10.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_name_match10(const char *rec10, const char *name)
        ;;
        ;; Arguments:
        ;;   HL = record filename field (10 bytes)
        ;;   DE = user filename pointer (C string or 10-char padded)
        ;;
        ;; Returns:
        ;;   A/L = 1 when names match, 0 otherwise
_mdr_name_match10::
        call    __mdr_name_match10
        jr      c,.nmw_miss
        ld      a,#1
        ld      l,a
        ret
.nmw_miss:
        xor     a
        ld      l,a
        ret

        ;; ------------------------------------------------------------
        ;; _mdr_make_name10
        ;; C-callable filename normalizer.
        ;;
        ;; Signature:
        ;;   void mdr_make_name10(const char *src, char out[10])
        ;;
        ;; Arguments:
        ;;   HL = src C string
        ;;   DE = output buffer (10 bytes)
_mdr_make_name10::
        push    de
        ld      b,#10
        ld      a,#' '
.mk_fill:
        ld      (de),a
        inc     de
        djnz    .mk_fill
        pop     de
        ld      b,#10
.mk_copy:
        ld      a,(hl)
        or      a
        ret     z
        ld      (de),a
        inc     hl
        inc     de
        djnz    .mk_copy
        ret

        ;; ------------------------------------------------------------
        ;; _mdr_find_file_size
        ;; C-callable directory scan helper.
        ;;
        ;; Signature:
        ;;   uint16_t mdr_find_file_size(uint8_t drive, const char *name10)
        ;;
        ;; Arguments:
        ;;   A  = drive number (1-8)
        ;;   DE = pointer to 10-char name
        ;;
        ;; Returns:
        ;;   DE = file size in bytes, 0 when not found
_mdr_find_file_size::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de                      ; save name pointer
        dec     sp
        ld      hl,#0
        add     hl,sp
        ld      (hl),a                  ; save drive byte
        ld      hl,#-448                ; local files[32]
        add     hl,sp
        ld      sp,hl
        ld      hl,#0
        add     hl,sp
        ex      de,hl                   ; DE = files buffer
        dec     sp
        ld      a,#32
        ld      hl,#0
        add     hl,sp
        ld      (hl),a                  ; max entries for mdr_dir
        ld      a,-3(ix)                ; drive
        call    _mdr_dir
        ld      b,a                     ; count
        ld      hl,#0
        add     hl,sp                   ; HL = files base
.ffs_loop:
        ld      a,b
        or      a
        jr      z,.ffs_not_found
        ld      e,-2(ix)                ; name pointer
        ld      d,-1(ix)
        push    hl
        call    _mdr_name_match10
        pop     hl
        or      a
        jr      nz,.ffs_found
        ld      de,#14
        add     hl,de                   ; next file entry
        djnz    .ffs_loop
.ffs_not_found:
        ld      de,#0
        ld      hl,#451                 ; drop files + saved name + drive
        add     hl,sp
        ld      sp,hl
        pop     ix
        ret
.ffs_found:
        ld      de,#12
        add     hl,de                   ; size offset
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      hl,#451                 ; drop files + saved name + drive
        add     hl,sp
        ld      sp,hl
        pop     ix
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_get_m_hd
        ;; Dispatch strategy:
        ;;   wait for GAP transition, sync to byte stream, read 15 bytes.
        ;;
        ;; Signature:
        ;;   carry = __mdr_get_m_hd(uint8_t *dest15)
        ;;
        ;; Arguments:
        ;;   HL = destination pointer (15 bytes)
__mdr_get_m_hd::
        ld	c,#MD_CTRL
        ld	de,#0x2000              ; overall timeout budget
.gm_chk_again:
        ld	b,#8
.gm_chk_loop:
        in	a,(c)
        and	#MD_R_GAP_ALT
        jr	z,.gm_dec_to_again
        djnz	.gm_chk_loop
.gm_chk_ag2:
        ld	b,#6
.gm_chk_lp2:
        in	a,(c)
        and	#MD_R_GAP_ALT
        jr	nz,.gm_dec_to_ag2
        djnz	.gm_chk_lp2
        ld	a,#MD_CTL_READ
        out	(#MD_CTRL),a
.gm_sync:
        in	a,(c)
        and	#MD_R_SYNC_ALT
        jr	z,.gm_read
        dec	de
        ld	a,d
        or	e
        jr	nz,.gm_sync
        jr	.gm_timeout
.gm_dec_to_again:
        dec	de
        ld	a,d
        or	e
        jr	nz,.gm_chk_again
        jr	.gm_timeout
.gm_dec_to_ag2:
        dec	de
        ld	a,d
        or	e
        jr	nz,.gm_chk_ag2
        jr	.gm_timeout
.gm_read:
        ld	b,#15
.gm_read_lp:
        in	a,(#MD_DATA)
        ld	(hl),a
        inc	hl
        djnz	.gm_read_lp
        or	a                       ; clear carry
        ret
.gm_timeout:
        scf
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_skip_payload
        ;; Dispatch strategy:
        ;;   discard the 512-byte data block plus trailing checksum byte.
        ;;
        ;; Signature:
        ;;   void __mdr_skip_payload(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_skip_payload::
        ld	b,#0                    ; first 256 bytes
.sp_loop1:
        call	__mdr_read_byte
        djnz	.sp_loop1
        ld	b,#0                    ; second 256 bytes
.sp_loop2:
        call	__mdr_read_byte
        djnz	.sp_loop2
        call	__mdr_read_byte         ; checksum byte
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_write_hdr
        ;; Dispatch strategy:
        ;;   stream the prepared 15-byte sector header from header_buf.
        ;;
        ;; Signature:
        ;;   void __mdr_write_hdr(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_write_hdr::
        ld	hl,#header_buf
        ld	b,#15
.wh_loop:
        ld	a,(hl)
        call	__mdr_write_byte
        inc	hl
        djnz	.wh_loop
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_write_rec
        ;; Dispatch strategy:
        ;;   build record descriptor from save_* state and emit 15 bytes.
        ;;
        ;; Signature:
        ;;   void __mdr_write_rec(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_write_rec::
        ld	hl,#rec_buf
        ld	bc,(save_len)
        ld	a,b
        cp	#2
        jr	nz,.wr_flag_last
        ld	a,c
        or	a
        ld	a,#0x04                 ; full 512-byte record
        jr	z,.wr_flag_set
.wr_flag_last:
        ld	a,#0x06                 ; final short record
.wr_flag_set:
        ld	(hl),a
        inc	hl
        ld	a,(save_rec_num)
        ld	(hl),a                  ; record number
        inc	hl
        ld	bc,(save_len)
        ld	(hl),c                  ; length low
        inc	hl
        ld	(hl),b                  ; length high
        inc	hl
        ld	de,(save_name)          ; copy/pad filename to 10 chars
        ld	b,#10
        ld	c,#0                    ; trailing-space padding mode
.wr_name:
        ld	a,c
        or	a
        jr	nz,.wr_name_pad
        ld	a,(de)
        or	a
        jr	nz,.wr_name_have
        inc	c
.wr_name_pad:
        ld	a,#' '
        jr	.wr_name_store
.wr_name_have:
        inc	de
.wr_name_store:
        ld	(hl),a
        inc	hl
        djnz	.wr_name
        ld	hl,#rec_buf
        ld	b,#14
        call	__mdr_calc_checksum
        ld	(rec_buf + REC_CHK),a
        ld	hl,#rec_buf
        ld	b,#15
.wr_emit:
        ld	a,(hl)
        call	__mdr_write_byte
        inc	hl
        djnz	.wr_emit
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_write_data
        ;; Dispatch strategy:
        ;;   write payload, pad to 512 bytes, then emit checksum byte.
        ;;
        ;; Signature:
        ;;   void __mdr_write_data(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_write_data::
        ld	hl,(save_src)
        ld	bc,(save_len)
        ld	e,#0                    ; running checksum accumulator
.wd_copy:
        ld	a,b
        or	c
        jr	z,.wd_pad_prep
        ld	a,(hl)
        call	__mdr_write_byte
        add	a,e
        ld	e,a
        inc	hl
        dec	bc
        jr	.wd_copy
.wd_pad_prep:
        ld	hl,#512
        ld	bc,(save_len)
        xor	a
        sbc	hl,bc                   ; HL = bytes to pad
        jr	z,.wd_checksum
.wd_pad:
        xor	a
        call	__mdr_write_byte
        dec	hl
        ld	a,h
        or	l
        jr	nz,.wd_pad
.wd_checksum:
        ld	a,e
        and	#0xfe                   ; keep checksum out of 0xff
        call	__mdr_write_byte
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_write_byte
        ;; Dispatch strategy:
        ;;   write one byte to MD_DATA and wait a short settle period.
        ;;
        ;; Signature:
        ;;   void __mdr_write_byte(uint8_t b)
        ;;
        ;; Arguments:
        ;;   A = byte to write
__mdr_write_byte::
        out	(#MD_DATA),a
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_select_drive
        ;; Dispatch strategy:
        ;;   run IF1 SW-MOTOR sequence across 8 drive slots.
        ;;
        ;; Signature:
        ;;   void __mdr_select_drive(uint8_t drive)
        ;;
        ;; Arguments:
        ;;   A = drive number (0-8), 0 = all off
__mdr_select_drive::
        call	_enter_critical_section
        push	bc
        push	de
        push	af
        ld	d,#0x01                 ; data bit sent to MD_SEL when selecting
        ld	e,#0x00                 ; data bit for non-selected drives
        ld	c,a                     ; c = target drive
        ld	a,#9
        sub	a,c
        ld	c,a                     ; c = 9 - drive
        ld	b,#8
.sel_loop:
        dec	c
        jr	nz,.off_motor
        ld	a,d
        out	(#MD_SEL),a
        ld	a,#MD_CTL_READ
        out	(#MD_CTRL),a
        call	__mdr_delay_1ms
        ld	a,#0xEC
        out	(#MD_CTRL),a
        call	__mdr_delay_1ms
        jr	.next_motor
.off_motor:
        ld	a,#0xEF
        out	(#MD_CTRL),a
        ld	a,e
        out	(#MD_SEL),a
        call	__mdr_delay_1ms
        ld	a,#0xED
        out	(#MD_CTRL),a
        call	__mdr_delay_1ms
.next_motor:
        djnz	.sel_loop
        pop	af                      ; restore requested drive (0..8)
        or	a
        jr	z,.sel_done             ; drive 0 => deselect all, no final select pulse
        ld	a,d                     ; finalize selected-drive latch only for 1..8
        out	(#MD_SEL),a
        ld	a,#MD_CTL_READ
        out	(#MD_CTRL),a
.sel_done:
        pop	de
        pop	bc
        call	_leave_critical_section
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_start_motor
        ;; Dispatch strategy:
        ;;   assert IF1 read mode for currently selected drive.
        ;;
        ;; Signature:
        ;;   void __mdr_start_motor(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_start_motor::
        ld	a,#MD_CTL_READ           ; read mode
        out	(#MD_CTRL),a
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_stop_motor
        ;; Dispatch strategy:
        ;;   deselect drives, settle control lines, then return to read mode.
        ;;
        ;; Signature:
        ;;   void __mdr_stop_motor(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_stop_motor::
        push	bc
        xor	a
        out	(#MD_SEL),a             ; clear select latch first
        ld	a,#0xEF                 ; stop pulse (clock high)
        out	(#MD_CTRL),a
        call	__mdr_delay_1ms
        ld	a,#0xED                 ; stop pulse (clock low)
        out	(#MD_CTRL),a
        call	__mdr_delay_1ms
        ld	a,#MD_CTL_READ          ; leave IF1 in safe read state
        out	(#MD_CTRL),a
        pop	bc
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_wait_gap_sync
        ;; Dispatch strategy:
        ;;   poll CTRL bits until GAP+SYNC appears or timeout expires.
        ;;
        ;; Signature:
        ;;   carry = __mdr_wait_gap_sync(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_wait_gap_sync::
        push	de
        ld	bc,#MD_CTRL
        ld	de,#0x0400              ; timeout budget
.wgs_poll_entry:
.wgs_poll:
        in	a,(c)
        ld	b,a
        and	#MD_R_GAPSYNC
        cp	#MD_R_GAPSYNC
        jr	z,.wgs_ok
        ld	a,b
        and	#MD_R_GAPSYNC_ALT
        cp	#MD_R_GAPSYNC_ALT
        jr	z,.wgs_ok
.wgs_wait:
        dec	de
        ld	a,d
        or	e
        jr	nz,.wgs_poll
        scf                             ; timed out
        pop	de
        ret
.wgs_ok:
        or	a                       ; clear carry on success
        pop	de
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_wait_gap_sync_long
        ;; Dispatch strategy:
        ;;   same as __mdr_wait_gap_sync, but with a longer timeout so save
        ;;   can wait across a whole data block for the next sector header.
        ;;
        ;; Signature:
        ;;   carry = __mdr_wait_gap_sync_long(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_wait_gap_sync_long::
        push	de
        ld	bc,#MD_CTRL
        ld	de,#0x6000              ; long timeout for post-record scan
        jr	.wgs_poll_entry

        ;; ------------------------------------------------------------
        ;; __mdr_read_byte
        ;; Dispatch strategy:
        ;;   single direct read from MD_DATA.
        ;;
        ;; Signature:
        ;;   uint8_t __mdr_read_byte(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_read_byte::
        in	a,(#MD_DATA)
        or	a                       ; clear carry
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_wait_sync
        ;; Dispatch strategy:
        ;;   wait for a SYNC edge change relative to baseline level.
        ;;
        ;; Signature:
        ;;   carry = __mdr_wait_sync(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_wait_sync::
        push	de
        ld	bc,#MD_CTRL
        ld	de,#0x0400              ; timeout budget
        in	a,(c)
        and	#MD_R_SYNC_ANY
        ld	b,a                     ; baseline sync level
.ws_wait:
        in	a,(c)
        and	#MD_R_SYNC_ANY
        cp	b
        jr	nz,.ws_ok
        dec	de
        ld	a,d
        or	e
        jr	nz,.ws_wait
        scf                             ; timed out
        pop	de
        ret
.ws_ok:
        or	a                       ; clear carry
        pop	de
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_calc_checksum
        ;; Dispatch strategy:
        ;;   accumulate modulo 255 while keeping result in 0..254.
        ;;
        ;; Signature:
        ;;   uint8_t __mdr_calc_checksum(const uint8_t *src, uint8_t len)
        ;;
        ;; Arguments:
        ;;   HL = pointer to data block
        ;;   B  = byte count
__mdr_calc_checksum::
        xor	a                       ; accumulator = 0 (remainder mod 255)
.cs_loop:
        ld	c,(hl)                   ; next byte
        add	a,c                      ; add modulo 256, carry means +256
        jr	nc,.cs_chk255
        sub	#0xff                   ; carry path: equivalent to +1 (subtract 255)
.cs_chk255:
        cp	#0xff                   ; exactly 255?
        jr	c,.cs_next
        sub	#0xff                   ; 255 -> 0
.cs_next:
        inc	hl
        djnz	.cs_loop
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_delay_1ms
        ;; Dispatch strategy:
        ;;   fixed countdown loop tuned for approximately 1 ms.
        ;;
        ;; Signature:
        ;;   void __mdr_delay_1ms(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_delay_1ms::
        push	bc
        ld	bc,#0x0087
.d1ms:
        dec	bc
        ld	a,b
        or	c
        jr	nz,.d1ms
        pop	bc
        ret

        .area	_BSS

load_fname::
        .ds	2                       ; saved filename pointer (mdr_load)
load_dest::
        .ds	2                       ; destination base pointer (mdr_load)
load_found::
        .ds	1                       ; at least one matching record loaded
slice_off::
        .ds	2                       ; requested file offset (mdr_load_slice)
slice_end::
        .ds	2                       ; requested end offset (mdr_load_slice)
slice_cur::
        .ds	2                       ; current file offset (mdr_load_slice)
dir_buf_ptr::
        .ds	2                       ; result buffer pointer (mdr_dir)
dir_file_cnt::
        .ds	1                       ; entries filled (mdr_dir)
header_buf::
        .ds	15                      ; sector header scratch buffer
save_hdr_buf::
        .ds	15                      ; preserved target header for mdr_save
rec_buf::
        .ds	15                      ; record descriptor scratch buffer
save_name::
        .ds	2                       ; saved filename pointer (mdr_save)
save_src::
        .ds	2                       ; saved source pointer (mdr_save)
save_len::
        .ds	2                       ; chunk byte length (mdr_save)
save_rem::
        .ds	2                       ; remaining byte length (mdr_save)
save_rec_num::
        .ds	1                       ; current record number (mdr_save)
save_sec_num::
        .ds	1                       ; selected free sector number (mdr_save)
