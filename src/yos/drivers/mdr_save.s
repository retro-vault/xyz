        ;; mdr_save.s
        ;;
        ;; ZX Spectrum Microdrive driver: save entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_save

        .globl  __mdr_select_drive
        .globl  __mdr_start_motor
        .globl  __mdr_wait_gap_sync
        .globl  __mdr_read_hdr_rec
        .globl  __mdr_write_hdr
        .globl  __mdr_write_rec
        .globl  __mdr_write_data
        .globl  __mdr_stop_motor

        .equ    REC_FLAG, 0

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
