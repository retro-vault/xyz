        ;; mdr_common.s
        ;;
        ;; ZX Spectrum Microdrive driver: shared helpers/state.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_common

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
        .equ    REC_CHK, 14             ; uint8_t checksum of bytes 0..13
        .equ    MDR_DBG_SZ, 10          ; sizeof(mdr_debug_t)

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; __mdr_dbg_reset
        ;; Dispatch strategy:
        ;;   clear the fixed-size debug snapshot buffer in one pass.
        ;;
        ;; Signature:
        ;;   void __mdr_dbg_reset(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_dbg_reset::
        ld	hl,#dbg_op
        ld	b,#MDR_DBG_SZ
        xor	a
.dbg_reset:
        ld	(hl),a
        inc	hl
        djnz	.dbg_reset
        ret

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
        ld	c,#MD_DATA
        ld	b,#15
.gm_read_lp:
        in	a,(c)
        ld	(hl),a
        inc	hl
        djnz	.gm_read_lp
        or	a                       ; clear carry
        ret
.gm_timeout:
        scf
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
        ld	a,#MD_W_ERASE           ; write mode + erase
        out	(#MD_CTRL),a
        call	__mdr_delay_10us
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
        ld	a,#MD_W_ERASE           ; write mode + erase
        out	(#MD_CTRL),a
        ld	hl,#rec_buf
        ld	(hl),#1                 ; flag: sector in use
        inc	hl
        ld	a,(save_rec_num)
        ld	(hl),a                  ; record number
        inc	hl
        ld	bc,(save_len)
        ld	(hl),c                  ; length low
        inc	hl
        ld	(hl),b                  ; length high
        inc	hl
        ld	de,(save_name)          ; copy 10-char padded filename
        ld	b,#10
.wr_name:
        ld	a,(de)
        ld	(hl),a
        inc	hl
        inc	de
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
        ld	a,#MD_W_ERASE           ; write mode + erase
        out	(#MD_CTRL),a
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
        push	bc
        ld	c,#MD_DATA
        out	(c),a
        call	__mdr_delay_short
        pop	bc
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
        call	_ir_disable
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
        call	_ir_enable
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
        ;;   write zero to control port to stop motor and deselect drives.
        ;;
        ;; Signature:
        ;;   void __mdr_stop_motor(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_stop_motor::
        xor	a                       ; all motors off
        out	(#MD_CTRL),a
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
        push	hl
        ld	hl,#dbg_gap_timeouts
        inc	(hl)
        pop	hl
        scf                             ; timed out
        pop	de
        ret
.wgs_ok:
        or	a                       ; clear carry on success
        pop	de
        ret

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
        push	bc
        ld	c,#MD_DATA
        in	a,(c)
        pop	bc
        or	a                       ; clear carry
        ret
.rb_timeout:
        xor	a                       ; timeout => deterministic filler
        scf
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
        push	hl
        ld	hl,#dbg_byte_timeouts
        inc	(hl)
        pop	hl
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
        push	bc
        push	hl
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
        pop	hl
        pop	bc
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

        ;; ------------------------------------------------------------
        ;; __mdr_delay_10us
        ;; Dispatch strategy:
        ;;   fixed short loop for approximately 10 us.
        ;;
        ;; Signature:
        ;;   void __mdr_delay_10us(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_delay_10us::
        push	bc
        ld	b,#6                    ; ~10us at 3.5MHz
.d10us:
        djnz	.d10us
        pop	bc
        ret

        ;; ------------------------------------------------------------
        ;; __mdr_delay_short
        ;; Dispatch strategy:
        ;;   fixed tiny loop for write-settle timing.
        ;;
        ;; Signature:
        ;;   void __mdr_delay_short(void)
        ;;
        ;; Arguments:
        ;;   (none)
__mdr_delay_short::
        push	bc
        ld	b,#12                   ; short settling delay
.dshrt:
        djnz	.dshrt
        pop	bc
        ret

        .area	_BSS

dbg_op::
        .ds	1                       ; last operation (1=dir)
dbg_drive::
        .ds	1                       ; last selected drive
dbg_scanned::
        .ds	1                       ; sectors scanned
dbg_aligned::
        .ds	1                       ; sectors with valid hdr+rec checksums
dbg_used::
        .ds	1                       ; used records with non-blank filename
dbg_blank::
        .ds	1                       ; used records with blank filename
dbg_align_fail::
        .ds	1                       ; checksum alignment failures
dbg_gap_timeouts::
        .ds	1                       ; sync wait timeouts (sector boundary)
dbg_byte_timeouts::
        .ds	1                       ; byte SYNC wait timeouts
dbg_result::
        .ds	1                       ; op result (dir count)

load_fname::
        .ds	2                       ; saved filename pointer (mdr_load)
load_dest::
        .ds	2                       ; destination base pointer (mdr_load)
load_found::
        .ds	1                       ; at least one matching record loaded
dir_buf_ptr::
        .ds	2                       ; result buffer pointer (mdr_dir)
dir_buf_max::
        .ds	1                       ; max entries (mdr_dir)
dir_file_cnt::
        .ds	1                       ; entries filled (mdr_dir)
scan_ptr::
        .ds	2                       ; candidate header pointer in scan window
scan_buf::
        .ds	96                      ; stream window for auto-alignment
load_sec_buf::
        .ds	512                     ; captured payload block (mdr_load)
header_buf::
        .ds	15                      ; sector header scratch buffer
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
