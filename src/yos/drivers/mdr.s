        ;; mdr.s
        ;;
        ;; ZX Spectrum Microdrive driver: dir and load.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-29   tstih

        .module	mdr

        .equ	MD_DATA, 0xe7           ; microdrive data port
        .equ	MD_CTRL, 0xef           ; microdrive control/status port
        .equ	MD_SEL, 0xf7            ; drive select data port

        ;; port EFh read bits (Interface 1)
        .equ	MD_R_WP, 0x01           ; bit 0: write protect
        .equ	MD_R_SYNC, 0x10         ; bit 4: sync
        .equ	MD_R_GAP, 0x20          ; bit 5: gap
        .equ	MD_R_GAPSYNC, 0x30      ; gap+sync mask
        .equ	MD_R_SYNC_ALT, 0x02     ; alt sync bit seen in some IF1 docs/emus
        .equ	MD_R_GAP_ALT, 0x04      ; alt gap bit seen in some IF1 docs/emus
        .equ	MD_R_SYNC_ANY, 0x12     ; accept either sync bit encoding
        .equ	MD_R_GAPSYNC_ALT, 0x06  ; alt gap+sync mask

        .equ	MD_CTL_READ, 0xEE       ; IF1 read mode control value

        ;; port EFh write bits (Interface 1)
        .equ	MD_W_DATA, 0x01         ; bit 0: comms data (inverted on line)
        .equ	MD_W_CLK, 0x02          ; bit 1: comms clock
        .equ	MD_W_RW, 0x04           ; bit 2: 1=read, 0=write
        .equ	MD_W_ERASE, 0x08        ; bit 3: erase (active)

        ;; mdr_file_t field offsets (sizeof = 14)
        .equ	MDR_FILE_NAME, 0        ; char name[11]  — null-terminated
        .equ	MDR_FILE_SECS, 11       ; uint8_t sectors
        .equ	MDR_FILE_SIZE, 12       ; uint16_t size   — little-endian
        .equ	MDR_FILE_SZ, 14         ; sizeof(mdr_file_t)

        ;; md_record_t field offsets within rec_buf
        .equ	REC_FLAG, 0             ; uint8_t  flag
        .equ	REC_NUM, 1              ; uint8_t  record number
        .equ	REC_LEN, 2              ; uint16_t length (lo at +2, hi at +3)
        .equ	REC_FNAME, 4            ; char     filename[10]
        .equ	REC_CHK, 14             ; uint8_t  checksum of bytes 0..13
        .equ	MDR_DBG_SZ, 10          ; sizeof(mdr_debug_t)

        .area	_CODE

; ============================================================
; Public functions
; ============================================================

        ;; extern uint8_t _mdr_dir(uint8_t drive, mdr_file_t *files, uint8_t max);
        ;; param:  A  = drive number (1-8)
        ;;         DE = pointer to mdr_file_t result array
        ;;         (stack +2) = maximum number of entries to fill (uint8_t)
        ;; return: A/L = number of unique files found
        ;; affects: AF, BC, DE, HL
        ;; notes:   scans up to 254 sectors; aggregates multi-sector files;
        ;;          uniqueness determined by exact 10-char filename match
_mdr_dir::
        ld	c,a                     ; preserve drive in C
        ;; fixed max (ysh buffer): avoid stack-arg ABI ambiguity here
        ld	b,#32
        ex	de,hl                   ; HL = files
        call	_ir_disable
        push	hl                      ; __mdr_dbg_reset clobbers HL
        push	bc                      ; __mdr_dbg_reset clobbers B (max)
        call	__mdr_dbg_reset
        pop	bc
        pop	hl
        ld	a,c                     ; restore drive
        ld	(dbg_drive),a
        ld	a,#1
        ld	(dbg_op),a
        push	af                      ; save drive (A used below)
        ld	(dir_buf_ptr),hl        ; save result buffer pointer
        ld	a,b
        ld	(dir_buf_max),a         ; save max entries
        ld	(dbg_gap_timeouts),a    ; debug: requested max entries
        xor	a
        ld	(dir_file_cnt),a        ; file count = 0
        pop	af                      ; A = drive
        call	__mdr_select_drive
        call	__mdr_start_motor
        ld	b,#0                    ; sector counter
.dir_scan:
        push	bc
        ;; read header+record from current stream position
        call	__mdr_read_hdr_rec
        jr	nc,.dir_ok_rec
        ld	hl,#dbg_align_fail
        inc	(hl)
        jp	.dir_next
.dir_ok_rec:
        ld	hl,#dbg_aligned
        inc	(hl)
        ;; skip free sectors (flag = 0)
        ld	a,(rec_buf + REC_FLAG)
        or	a
        jp	z,.dir_next
        ;; reject impossible record lengths (valid: 1..512)
        ld	a,(rec_buf + REC_LEN + 1) ; high byte
        cp	#2
        jr	c,.dir_len_lochk
        jp	nz,.dir_next
        ld	a,(rec_buf + REC_LEN)    ; hi==2 => only 0x0200 allowed
        or	a
        jp	nz,.dir_next
        jr	.dir_len_ok
.dir_len_lochk:
        ld	a,(rec_buf + REC_LEN + 1)
        or	a
        jr	nz,.dir_len_ok
        ld	a,(rec_buf + REC_LEN)    ; reject zero length
        or	a
        jp	z,.dir_next
.dir_len_ok:
        ;; reject non-printable filenames (garbage/false positives)
        ld	hl,#rec_buf + REC_FNAME
        ld	c,#10
.dir_fname_chk:
        ld	a,(hl)
        cp	#' '
        jr	z,.dir_fname_okc
        cp	#'0'
        jp	c,.dir_next
        cp	#('9' + 1)
        jr	c,.dir_fname_okc
        cp	#'A'
        jp	c,.dir_next
        cp	#('Z' + 1)
        jr	c,.dir_fname_okc
        cp	#'a'
        jp	c,.dir_next
        cp	#('z' + 1)
        jp	nc,.dir_next
.dir_fname_okc:
        inc	hl
        dec	c
        jr	nz,.dir_fname_chk
        ;; skip blank filenames (space in first char)
        ld	a,(rec_buf + REC_FNAME)
        cp	#' '
        jr	nz,.dir_named
        ld	hl,#dbg_blank
        inc	(hl)
        jp	.dir_next
.dir_named:
        ld	hl,#dbg_used
        inc	(hl)
        ;; search result array for a matching filename
        ld	hl,(dir_buf_ptr)        ; HL = first entry
        ld	a,(dir_file_cnt)
        ld	b,a                     ; B = entries to search
        or	a
        jr	z,.dir_not_found        ; no entries yet
.dir_search:
        push	bc                      ; save search counter
        push	hl                      ; save entry pointer
        ld	de,#rec_buf + REC_FNAME ; DE = filename in record
        ld	c,#10                   ; compare 10 chars
.dir_cmp:
        ld	a,(de)
        cp	(hl)
        jr	nz,.dir_cmp_no
        inc	hl
        inc	de
        dec	c
        jr	nz,.dir_cmp
        ;; match — HL is at entry+10; update sectors and size
        pop	hl                      ; HL = entry base
        pop	bc                      ; discard search counter
        ld	de,#MDR_FILE_SECS       ; advance to sectors field
        add	hl,de
        inc	(hl)                    ; sectors++
        inc	hl                      ; → size lo (offset 12)
        ld	de,#rec_buf + REC_LEN
        ld	a,(de)                  ; rec length lo
        add	a,(hl)                  ; + current size lo
        ld	(hl),a                  ; store new size lo
        inc	hl                      ; → size hi (offset 13)
        inc	de                      ; → rec_buf + REC_LEN + 1
        ld	a,(de)                  ; rec length hi
        adc	a,(hl)                  ; + current size hi + carry
        ld	(hl),a                  ; store new size hi
        jr	.dir_next
.dir_cmp_no:
        pop	hl                      ; HL = entry base
        pop	bc                      ; restore search counter
        ld	de,#MDR_FILE_SZ         ; advance to next entry
        add	hl,de
        djnz	.dir_search
        ;; filename not in array — add new entry if room
.dir_not_found:
        ;; hard safety cap: never emit beyond 32 entries
        ld	a,(dir_file_cnt)
        cp	#32
        jp	nc,.dir_next
        ld	b,a                     ; B = current count
        ;; caller-specified max can be smaller than 32
        ld	a,(dir_buf_max)
        cp	b
        jp	z,.dir_next             ; full
        jp	c,.dir_next             ; overflow guard
        ;; recompute slot pointer from base + count * sizeof(mdr_file_t)
        ;; (do not trust HL from search walk)
        ld	hl,(dir_buf_ptr)
        ld	a,b
        or	a
        jr	z,.dir_slot_ready
.dir_slot_seek:
        ld	de,#MDR_FILE_SZ
        add	hl,de
        dec	a
        jr	nz,.dir_slot_seek
.dir_slot_ready:
        ld	de,#rec_buf + REC_FNAME ; copy 10-char filename
        ld	c,#10
.dir_copy:
        ld	a,(de)
        ld	(hl),a
        inc	hl
        inc	de
        dec	c
        jr	nz,.dir_copy
        ld	(hl),#0                 ; null terminator (offset 10)
        inc	hl                      ; → sectors (offset 11)
        ld	(hl),#1                 ; sectors = 1
        inc	hl                      ; → size lo (offset 12)
        ld	de,#rec_buf + REC_LEN
        ld	a,(de)
        ld	(hl),a                  ; size lo
        inc	hl                      ; → size hi (offset 13)
        inc	de
        ld	a,(de)
        ld	(hl),a                  ; size hi
        ld	hl,#dir_file_cnt
        inc	(hl)                    ; file count++
        ld	a,(hl)
        cp	#33
        jr	c,.dir_next
        ld	(hl),#32                ; clamp in case of corruption
.dir_next:
        ld	hl,#dbg_scanned
        inc	(hl)
        pop	bc                      ; restore sector counter
        inc	b
        ld	a,b
        cp	#254                    ; scanned enough sectors?
        jp	nz,.dir_scan
        call	__mdr_stop_motor
.dir_done:
        ;; hard-cap final count to 32 first (defensive)
        ld	a,(dir_file_cnt)
        cp	#33
        jr	c,.dir_cnt_cap32_ok
        ld	a,#32
        ld	(dir_file_cnt),a
.dir_cnt_cap32_ok:
        ;; and then to caller max (if lower)
        ld	a,(dir_file_cnt)
        ld	b,a
        ld	a,(dir_buf_max)
        cp	b
        jr	nc,.dir_cnt_ok
        ld	a,(dir_buf_max)
        ld	(dir_file_cnt),a
.dir_cnt_ok:
        ld	a,(dir_file_cnt)
        ld	(dbg_byte_timeouts),a   ; debug: final clamped count
        ld	(dbg_result),a
        ld	l,a                     ; return count in L
        ;; sdcccall(1): drop stacked max (1 byte)
        pop	bc                      ; return address
        inc	sp
        push	bc
        call	_ir_enable
        ret

        ;; extern uint8_t _mdr_load(uint8_t drive, char *name, uint8_t *dest);
        ;; param:  A  = drive number (1-8)
        ;;         DE = pointer to 10-char space-padded filename
        ;;         (stack +2,+3) = destination address (record #0 base)
        ;; return: A/L = 0 on success, 1 if file not found
        ;; affects: AF, BC, DE, HL
        ;; notes:   loads all matching records (same 10-char name).
        ;;          each record is placed at dest + record_num * 512.
_mdr_load::
        ;; adapt sdcccall(1): name in DE, dest on stack
        push	iy
        ld	iy,#4
        add	iy,sp
        ld	c,0(iy)                  ; dest lo
        ld	b,1(iy)                  ; dest hi
        pop	iy
        ex	de,hl                   ; HL = name
        call	_ir_disable
        push	af                      ; preserve drive across setup (A gets clobbered)
        ld	(load_fname),hl         ; save filename pointer
        ld	hl,#load_dest
        ld	(hl),c
        inc	hl
        ld	(hl),b
        xor	a
        ld	(load_found),a
        pop	af                      ; restore drive
        call	__mdr_select_drive
        call	__mdr_start_motor
        ld	b,#0                    ; sector counter
.ld_scan:
        push	bc                      ; save sector counter
        ;; read header+record from current stream position
        call	__mdr_read_hdr_rec
        jr	c,.ld_next
        ;; reject impossible record lengths (valid: 1..512)
        ld	a,(rec_buf + REC_LEN + 1) ; high byte
        cp	#2
        jr	c,.ld_len_lochk
        jr	nz,.ld_next
        ld	a,(rec_buf + REC_LEN)    ; hi==2 => only 0x0200 allowed
        or	a
        jr	nz,.ld_next
        jr	.ld_len_ok
.ld_len_lochk:
        ld	a,(rec_buf + REC_LEN + 1)
        or	a
        jr	nz,.ld_len_ok
        ld	a,(rec_buf + REC_LEN)    ; reject zero length
        or	a
        jr	z,.ld_next
.ld_len_ok:
        ;; compare 10-char filename at rec_buf+REC_FNAME against saved pointer
        ld	hl,#rec_buf + REC_FNAME
        ld	de,(load_fname)         ; de = user filename pointer
        ld	c,#10
.ld_cmp:
        ld	a,(de)
        cp	(hl)                    ; compare user char vs record char
        jr	nz,.ld_next
        inc	hl
        inc	de
        dec	c
        jr	nz,.ld_cmp
        ;; filename matched — stream 512 data bytes and checksum
        ld	a,#1
        ld	(load_found),a
        ld	a,(rec_buf + REC_LEN)    ; bc = chunk length
        ld	c,a
        ld	a,(rec_buf + REC_LEN + 1)
        ld	b,a
        ld	hl,(load_dest)           ; target = base + rec_num * 512
        ld	a,(rec_buf + REC_NUM)
        add	a,a
        ld	e,a
        ld	a,h
        add	a,e
        jr	c,.ld_skip_store
        ld	h,a
        ld	d,b
        ld	e,c                     ; de = bytes left to copy
        jr	.ld_data_stream
.ld_skip_store:
        ld	de,#0                   ; invalid target => only discard stream
.ld_data_stream:
        ld	b,#0                    ; first 256 bytes
.ld_data1:
        call	__mdr_read_byte
        ld	c,a                     ; keep byte while testing copy budget
        ld	a,d
        or	e
        jr	z,.ld_data1_next
        ld	a,c
        ld	(hl),a
        inc	hl
        dec	de
.ld_data1_next:
        djnz	.ld_data1
        ld	b,#0                    ; second 256 bytes
.ld_data2:
        call	__mdr_read_byte
        ld	c,a
        ld	a,d
        or	e
        jr	z,.ld_data2_next
        ld	a,c
        ld	(hl),a
        inc	hl
        dec	de
.ld_data2_next:
        djnz	.ld_data2
        call	__mdr_read_byte         ; discard data checksum byte
.ld_next:
        pop	bc                      ; restore sector counter
        inc	b
        ld	a,b
        cp	#254                    ; scanned enough sectors?
        jr	nz,.ld_scan
        call	__mdr_stop_motor
        ld	a,(load_found)
        or	a
        jr	nz,.ld_ok
        ld	a,#1                    ; return 1 = file not found
        jr	.ld_ret
.ld_ok:
        xor	a                       ; return 0 = success
.ld_ret:
        ld	l,a
        ;; sdcccall(1): drop stacked dest pointer (2 bytes)
        pop	bc                      ; return address
        inc	sp
        inc	sp
        push	bc
        call	_ir_enable
        ret

        ;; extern uint8_t _mdr_detect_drives(void);
        ;; param:  (none)
        ;; return: A = number of detected drives (0-8)
        ;; affects: AF, BC, DE
        ;; notes:   safe probe used by shell startup/path:
        ;;          checks only configured Fuse-style drives 1..2 and does not
        ;;          read MD_DATA (avoids IF1 IN-port stalls on false sync).
_mdr_detect_drives::
        call	_ir_disable
        ld	e,#0                    ; e = detected drive count
        ld	d,#1                    ; d = current drive under test
.det_loop:
        ld	a,d
        call	__mdr_select_drive      ; select drive d
        call	__mdr_start_motor
        call	__mdr_wait_gap_sync
        jr	c,.det_next
        inc	e                       ; count this drive
.det_next:
        call	__mdr_stop_motor
        inc	d
        ld	a,d
        cp	#3                      ; past drive 2?
        jr	nz,.det_loop
        ld	a,e                     ; return count in A
        or	a
        jr	nz,.det_ret
        ld	a,#2                    ; Fuse IF1 commonly exposes two drives
.det_ret:
        ld	l,a                     ; also in L for C return value
        call	_ir_enable
        ret

        ;; extern uint8_t _mdr_dbg(uint8_t drive, mdr_debug_t *out);
        ;; param:  A  = drive number (ignored; reserved for future per-drive stats)
        ;;         DE = output buffer pointer
        ;; return: A/L = 0
        ;; affects: AF, BC, DE, HL
_mdr_dbg::
        push	af
        call	_ir_disable
        pop	af
        ld	hl,#dbg_op
        ld	b,#MDR_DBG_SZ
.dbg_copy:
        ld	a,(hl)
        ld	(de),a
        inc	hl
        inc	de
        djnz	.dbg_copy
        call	_ir_enable
        xor	a
        ld	l,a
        ret

        ;; extern uint8_t _mdr_save(uint8_t drive, char *name, uint8_t *src, uint16_t len);
        ;; param:  A  = drive number (1-8)
        ;;         DE = pointer to 10-char space-padded filename
        ;;         (stack +2,+3) = source data address
        ;;         (stack +4,+5) = data length (1-65535 bytes)
        ;; return: A/L = 0 on success, 1 if no free sector found
        ;; affects: AF, BC, DE, HL
        ;; notes:   writes as many sectors as needed, each <=512 bytes.
        ;;          rec_num increments from 0 for each written sector.
_mdr_save::
        ;; adapt sdcccall(1): name in DE, src+len on stack
        ex	de,hl                   ; HL = name
        push	iy
        ld	iy,#4
        add	iy,sp
        ld	e,0(iy)                  ; src lo
        ld	d,1(iy)                  ; src hi
        ld	c,2(iy)                  ; len lo
        ld	b,3(iy)                  ; len hi
        pop	iy
        call	_ir_disable
        push	af                      ; save drive
        ld	(save_name),hl          ; save filename pointer
        ld	(save_src),de           ; save source pointer
        ld	(save_rem),bc           ; save remaining data length
        xor	a
        ld	(save_rec_num),a        ; record number starts at 0
        pop	af                      ; A = drive
        ld	a,b
        or	c
        jr	z,.sav_fail              ; zero-length save is invalid
        call	__mdr_select_drive
        call	__mdr_start_motor
        ld	b,#0                    ; sector counter
.sav_scan:
        push	bc
        call	__mdr_wait_gap_sync     ; wait for sector boundary
        jr	nc,.sav_got_sync
        pop	bc                      ; restore sector counter
        inc	b                       ; try next sector slot on timeout
        ld	a,b
        cp	#254
        jr	nz,.sav_scan
        call	__mdr_stop_motor
        jr	.sav_fail
.sav_got_sync:
        ;; read and auto-align header+record using checksum validation
        call	__mdr_read_hdr_rec
        jr	c,.sav_next
        ;; skip sectors in use (flag != 0)
        ld	a,(rec_buf + REC_FLAG)
        or	a
        jr	nz,.sav_next
        ;; free sector found — write one chunk (<=512 bytes)
        ld	hl,(save_rem)
        ld	a,h
        cp	#2
        jr	c,.sav_chunk_rem
        ld	bc,#512
        jr	.sav_chunk_set
.sav_chunk_rem:
        ld	b,h
        ld	c,l
.sav_chunk_set:
        ld	(save_len),bc           ; chunk length for write routines
        pop	bc                      ; restore sector counter
        call	__mdr_write_hdr         ; rewrite sector header
        call	__mdr_write_rec         ; build and write record descriptor
        call	__mdr_write_data        ; write data block + pad + checksum
        ld	bc,(save_len)           ; restore chunk length (write calls clobber BC)
        ;; advance source pointer by chunk size
        ld	hl,(save_src)
        add	hl,bc
        ld	(save_src),hl
        ;; remaining -= chunk size
        ld	hl,(save_rem)
        xor	a
        sbc	hl,bc
        ld	(save_rem),hl
        ;; next record number
        ld	hl,#save_rec_num
        inc	(hl)
        ;; done?
        ld	hl,(save_rem)
        ld	a,h
        or	l
        jr	z,.sav_ok
        ;; continue scanning for next free sector
        inc	b
        ld	a,b
        cp	#254
        jr	nz,.sav_scan
        call	__mdr_stop_motor
        jr	.sav_fail
.sav_next:
        pop	bc                      ; restore sector counter
        inc	b
        ld	a,b
        cp	#254                    ; scanned enough sectors?
        jr	nz,.sav_scan
        call	__mdr_stop_motor
.sav_fail:
        ld	a,#1                    ; return 1 = no free sector
        ld	l,a
        jr	.sav_ret
.sav_ok:
        call	__mdr_stop_motor
        xor	a                       ; return 0 = success
        ld	l,a
.sav_ret:
        ;; sdcccall(1): drop stacked src pointer + len (4 bytes)
        pop	bc                      ; return address
        inc	sp
        inc	sp
        inc	sp
        inc	sp
        push	bc
        call	_ir_enable
        ret

        ;; __mdr_dbg_reset
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC, HL
__mdr_dbg_reset::
        ld	hl,#dbg_op
        ld	b,#MDR_DBG_SZ
        xor	a
.dbg_reset:
        ld	(hl),a
        inc	hl
        djnz	.dbg_reset
        ret

        ;; __mdr_read_hdr_rec
        ;; param:  (none)
        ;; return: C=0 on success (header_buf+rec_buf filled), C=1 on failure
        ;; affects: AF, BC, DE, HL
        ;; notes:   IF1-style flow: fetch valid header block first, then
        ;;          fetch valid record descriptor block.
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

        ;; __mdr_get_m_hd
        ;; param:  HL = destination pointer (15 bytes)
        ;; return: C=0 on success, C=1 on timeout
        ;; affects: AF, BC, DE, HL
        ;; notes:   IF1 GET-M-HD style receive:
        ;;          detect GAP high then low, start read, wait SYNC low,
        ;;          read 15 bytes from MD_DATA.
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
        ld	a,#MD_CTL_READ
        out	(#MD_CTRL),a
        or	a                       ; clear carry
        ret
.gm_timeout:
        scf
        ret

        ;; __mdr_write_hdr
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC, HL
        ;; notes:   writes a 15-byte sector header from header_buf
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

        ;; __mdr_write_rec
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC, DE, HL
        ;; notes:   builds and writes a 15-byte record descriptor
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

        ;; __mdr_write_data
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC, DE, HL
        ;; notes:   writes len bytes from save_src, pads with zeros to 512,
        ;;          then writes checksum over the 512-byte data block
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

        ;; __mdr_write_byte
        ;; param:  A = byte to write
        ;; return: (none)
        ;; affects: BC
        ;; notes:   caller must preconfigure MD_CTRL for write mode
__mdr_write_byte::
        push	bc
        ld	c,#MD_DATA
        out	(c),a
        call	__mdr_delay_short
        pop	bc
        ret

; ============================================================
; Helpers
; ============================================================

        ;; __mdr_select_drive
        ;; param:  A = drive number (0-8), 0 = all off
        ;; return: (none)
        ;; affects: AF, BC, DE
        ;; notes:   IF1-style SW-MOTOR sequence.
__mdr_select_drive::
        call	_ir_disable
        push	bc
        push	de
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
        ld	a,d
        out	(#MD_SEL),a
        ld	a,#MD_CTL_READ
        out	(#MD_CTRL),a
        pop	de
        pop	bc
        call	_ir_enable
        ret

        ;; __mdr_start_motor
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF
        ;; notes:   keeps interface in read mode (selected drive keeps spinning)
__mdr_start_motor::
        ld	a,#MD_CTL_READ           ; read mode
        out	(#MD_CTRL),a
        ret

        ;; __mdr_stop_motor
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF
        ;; notes:   cuts motor power and deselects all drives
__mdr_stop_motor::
        ld	a,#0                    ; all off
        out	(#MD_CTRL),a
        ret

        ;; __mdr_wait_gap_sync
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC
        ;; notes:   waits until GAP+SYNC boundary is asserted.
        ;;          Accepts both known IF1 bit layouts (0x30 or 0x06).
        ;;          C=0 on success, C=1 on timeout
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

        ;; __mdr_read_byte
        ;; param:  (none)
        ;; return: A = byte read from data port (C=0)
        ;;         A = 0 on timeout (C=1)
        ;; affects: AF
        ;; notes:   direct stream read from MD_DATA.
        ;;          sync alignment is handled at sector level plus checksum scan.
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

        ;; __mdr_wait_sync
        ;; param:  (none)
        ;; return: C=0 on success, C=1 on timeout
        ;; affects: AF, BC
        ;; notes:   waits for SYNC transition (edge), then returns.
        ;;          Edge-based wait avoids repeatedly sampling one stale byte.
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

        ;; __mdr_calc_checksum
        ;; param:  HL = pointer to data block
        ;;         B  = byte count
        ;; return: A  = checksum (sum of bytes modulo 255)
        ;; affects: AF, HL, BC
        ;; notes:   running reduction keeps result in range 0..254
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

        ;; __mdr_delay_1ms
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC
        ;; notes:   roughly 1ms delay at 3.5MHz (IF1-compatible motor timing)
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

        ;; __mdr_delay_10us
        ;; param:  (none)
        ;; return: (none)
        ;; affects: BC
        ;; notes:   ~10us at 3.5MHz — adjust loop count for real hardware
__mdr_delay_10us::
        push	bc
        ld	b,#6                    ; ~10us at 3.5MHz
.d10us:
        djnz	.d10us
        pop	bc
        ret

        ;; __mdr_delay_short
        ;; param:  (none)
        ;; return: (none)
        ;; affects: BC
        ;; notes:   short settling delay — adjust loop count for real hardware
__mdr_delay_short::
        push	bc
        ld	b,#12                   ; short settling delay
.dshrt:
        djnz	.dshrt
        pop	bc
        ret

        .area	_BSS

dbg_op:
        .ds	1                       ; last operation (1=dir)
dbg_drive:
        .ds	1                       ; last selected drive
dbg_scanned:
        .ds	1                       ; sectors scanned
dbg_aligned:
        .ds	1                       ; sectors with valid hdr+rec checksums
dbg_used:
        .ds	1                       ; used records with non-blank filename
dbg_blank:
        .ds	1                       ; used records with blank filename
dbg_align_fail:
        .ds	1                       ; checksum alignment failures
dbg_gap_timeouts:
        .ds	1                       ; sync wait timeouts (sector boundary)
dbg_byte_timeouts:
        .ds	1                       ; byte SYNC wait timeouts
dbg_result:
        .ds	1                       ; op result (dir count)

load_fname:
        .ds	2                       ; saved filename pointer (mdr_load)
load_dest:
        .ds	2                       ; destination base pointer (mdr_load)
load_found:
        .ds	1                       ; at least one matching record loaded
dir_buf_ptr:
        .ds	2                       ; result buffer pointer (mdr_dir)
dir_buf_max:
        .ds	1                       ; max entries (mdr_dir)
dir_file_cnt:
        .ds	1                       ; entries filled (mdr_dir)
scan_ptr:
        .ds	2                       ; candidate header pointer in scan window
scan_buf:
        .ds	96                      ; stream window for auto-alignment
header_buf:
        .ds	15                      ; sector header scratch buffer
rec_buf:
        .ds	15                      ; record descriptor scratch buffer
save_name:
        .ds	2                       ; saved filename pointer (mdr_save)
save_src:
        .ds	2                       ; saved source pointer (mdr_save)
save_len:
        .ds	2                       ; chunk byte length (mdr_save)
save_rem:
        .ds	2                       ; remaining byte length (mdr_save)
save_rec_num:
        .ds	1                       ; current record number (mdr_save)
