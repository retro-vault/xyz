        ;; mdr_save.s
        ;;
        ;; ZX Spectrum Microdrive driver: save entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_save

        .globl  __mdr_select_drive
        .globl  __mdr_start_motor
        .globl  __mdr_wait_gap_sync_long
        .globl  __mdr_read_hdr_rec
        .globl  __mdr_name_match10
        .globl  __mdr_skip_payload
        .globl  __mdr_write_hdr
        .globl  __mdr_write_byte
        .globl  __mdr_write_rec
        .globl  __mdr_write_data
        .globl  __mdr_stop_motor

        .equ    REC_FLAG, 0
        .equ    HDR_SECTOR, 1
        .equ    REC_FNAME, 4

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; _mdr_save
        ;; Dispatch strategy:
        ;;   scan for free sectors and stream chunks (<=512 bytes) sequentially.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_save(uint8_t drive, char *name, uint8_t *src, uint16_t len)
        ;;
        ;; Arguments:
        ;;   A  = drive number (1-8)
        ;;   DE = pointer to 10-char space-padded filename
        ;;   stack: src, len
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
        call	_enter_critical_section
        push	af                      ; save drive
        ld	(save_name),hl          ; save filename pointer
        ld	(save_src),de           ; save source pointer
        ld	(save_rem),bc           ; save remaining data length
        xor	a
        ld	(save_rec_num),a        ; record number starts at 0
        ld	(save_have_free),a      ; no remembered free sector yet
        pop	af                      ; A = drive
        push	af                      ; preserve drive across debug/len checks
        ld	a,b
        or	c
        jr	nz,.sav_len_ok
        pop	af                      ; balance pushed drive on zero-length path
        jp	.sav_fail_zero          ; zero-length save is invalid
.sav_len_ok:
        pop	af                      ; restore drive for select_drive
        call	__mdr_select_drive
        call	__mdr_start_motor
        ld	b,#0                    ; sector counter
.sav_scan:
        push	bc
        call	__mdr_read_hdr_rec
        jr	nc,.sav_scan_ok
        jp	.sav_next
.sav_scan_ok:
        call	__mdr_skip_payload
        ld	a,(rec_buf + REC_FLAG)
        or	a
        jr	z,.sav_free
        ;; refuse duplicate filenames only before the first chunk is written.
        ;; once save_rec_num > 0, records with the same filename are the file
        ;; we are currently streaming across multiple sectors.
        ld	a,(save_rec_num)
        or	a
        jr	nz,.sav_next
        ld	hl,#rec_buf + REC_FNAME
        ld	de,(save_name)
        call	__mdr_name_match10
        jp	nc,.sav_fail_dup
        jp	.sav_next
.sav_free:
        call	.sav_capture_header
        ld	a,(save_rec_num)
        or	a
        jr	nz,.sav_free_commit
        ld	a,(save_have_free)
        or	a
        jr	nz,.sav_next
        ld	a,#1
        ld	(save_have_free),a
        jr	.sav_next
.sav_free_commit:
        pop	bc
.sav_have_target:
        xor	a
        ld	(save_have_free),a
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
        call	.sav_write_selected
        jr	nc,.sav_chunk_written
        ld	a,(save_rec_num)        ; first-chunk write failure is treated
        or	a                       ; as duplicate-name conflict
        jr	nz,.sav_fail
        call	__mdr_stop_motor
        ld	a,#2
        ld	l,a
        jr	.sav_ret
.sav_chunk_written:
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
        ld	b,#0
        jp	.sav_scan
.sav_next:
        pop	bc                      ; restore sector counter
        inc	b
        ld	a,b
        cp	#254                    ; scanned enough sectors?
        jp	nz,.sav_scan
        ld	a,(save_rec_num)
        or	a
        jr	nz,.sav_fail_endscan
        ld	a,(save_have_free)
        or	a
        jr	z,.sav_fail_endscan
        jr	.sav_have_target
.sav_fail_endscan:
.sav_fail:
        call	__mdr_stop_motor
        ld	a,#1                    ; return 1 = no free sector
        ld	l,a
        jr	.sav_ret
.sav_fail_zero:
        ld	a,#3                    ; return 3 = invalid length (zero)
        ld	l,a
        jr	.sav_ret
.sav_fail_dup:
        pop	bc
        call	__mdr_stop_motor
        ld	a,#2                    ; return 2 = name already exists
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
        call	_leave_critical_section
        ret

        ;; ------------------------------------------------------------
        ;; .sav_capture_header
        ;; Save current sector number and 15-byte header snapshot.
        ;;
        ;; Arguments:
        ;;   (none)
.sav_capture_header:
        ld	a,(header_buf + HDR_SECTOR)
        ld	(save_sec_num),a
        ld	hl,#header_buf
        ld	de,#save_hdr_buf
        ld	bc,#15
        ldir
        ret

        ;; ------------------------------------------------------------
        ;; .sav_write_selected
        ;; We can only write at the sector boundary, so after spotting a
        ;; free sector we wait until the sector before it passes, then arm
        ;; the writer at the next GAP+SYNC transition.
        ;;
        ;; Returns:
        ;;   carry clear = sector written
        ;;   carry set   = sector could not be aligned for write
.sav_write_selected:
        ld	a,(save_sec_num)
        cp	#254
        jr	z,.sav_prev_wrap
        inc	a
        jr	.sav_prev_ready
.sav_prev_wrap:
        ld	a,#1
.sav_prev_ready:
        ld	c,a                     ; C = sector number that precedes target
        ld	b,#0                    ; search budget
.sav_seek_prev:
        push	bc
        call	__mdr_read_hdr_rec
        pop	bc
        jr	c,.sav_seek_next
        ld	a,(header_buf + HDR_SECTOR)
        cp	c
        jr	z,.sav_wait_target
        call	__mdr_skip_payload
.sav_seek_next:
        inc	b
        ld	a,b
        cp	#254
        jr	nz,.sav_seek_prev
        scf
        ret
.sav_wait_target:
        ;; consume previous sector payload so head advances to the next
        ;; header boundary before waiting for GAP+SYNC.
        call	__mdr_skip_payload
        ;; restore preserved target header before entering the tight
        ;; boundary->write window below.
        ld	hl,#save_hdr_buf
        ld	de,#header_buf
        ld	bc,#15
        ldir
        call	__mdr_wait_gap_sync_long
        ret	c
        ;; block 1: preamble + 15-byte header
        ld	a,#0xea                 ; MD_CTL_WRITE_ERASE (clk high)
        out	(#0xef),a               ; MD_CTRL
        call	.sav_write_preamble
        call	__mdr_write_hdr
        ;; switch back to read mode and re-sync on block 2 boundary
        call	__mdr_start_motor
        call	__mdr_wait_gap_sync_long
        ret	c
        ;; block 2: preamble + record/data block
        ld	a,#0xea                 ; MD_CTL_WRITE_ERASE (clk high)
        out	(#0xef),a               ; MD_CTRL
        call	.sav_write_preamble
        call	__mdr_write_rec
        call	__mdr_write_data
        call	__mdr_start_motor
        or	a                       ; clear carry
        ret

        ;; ------------------------------------------------------------
        ;; .sav_write_preamble
        ;; Emit IF1 preamble: 10x00 then 2xFF.
        ;;
        ;; Arguments:
        ;;   (none)
.sav_write_preamble:
        ld	b,#10
.sav_pre0:
        xor	a
        call	__mdr_write_byte
        djnz	.sav_pre0
        ld	b,#2
.sav_preff:
        ld	a,#0xff
        call	__mdr_write_byte
        djnz	.sav_preff
        ret

        .area   _BSS
save_have_free::
        .ds     1
