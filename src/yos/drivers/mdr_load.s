        ;; mdr_load.s
        ;;
        ;; ZX Spectrum Microdrive driver: load entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_load

        .globl  __mdr_select_drive
        .globl  __mdr_start_motor
        .globl  __mdr_wait_sync
        .globl  __mdr_wait_gap_sync
        .globl  __mdr_read_hdr_rec
        .globl  __mdr_read_byte
        .globl  __mdr_delay_1ms
        .globl  __mdr_stop_motor

        .equ    REC_NUM, 1
        .equ    REC_LEN, 2
        .equ    REC_FNAME, 4

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; _mdr_load
        ;; Dispatch strategy:
        ;;   scan all sectors, copy data for matching filenames by record number.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_load(uint8_t drive, char *name, uint8_t *dest)
        ;;
        ;; Arguments:
        ;;   A  = drive number (1-8)
        ;;   DE = pointer to 10-char space-padded filename
        ;;   stack: dest
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
        jp	c,.ld_next
        ;; capture payload stream: 512 data bytes + 1 checksum byte
        ld	hl,#load_sec_buf
        ld	b,#0                    ; first 256 bytes
.ld_cap1:
        call	__mdr_read_byte
        ld	(hl),a
        inc	hl
        djnz	.ld_cap1
        ld	b,#0                    ; second 256 bytes
.ld_cap2:
        call	__mdr_read_byte
        ld	(hl),a
        inc	hl
        djnz	.ld_cap2
        call	__mdr_read_byte         ; discard data checksum byte
        ;; reject impossible record lengths (valid: 1..512)
        ld	a,(rec_buf + REC_LEN + 1) ; high byte
        cp	#2
        jr	c,.ld_len_lochk
        jp	nz,.ld_next
        ld	a,(rec_buf + REC_LEN)    ; hi==2 => only 0x0200 allowed
        or	a
        jp	nz,.ld_next
        jr	.ld_len_ok
.ld_len_lochk:
        ld	a,(rec_buf + REC_LEN + 1)
        or	a
        jr	nz,.ld_len_ok
        ld	a,(rec_buf + REC_LEN)    ; reject zero length
        or	a
        jp	z,.ld_next
.ld_len_ok:
        ;; compare filename (accept both padded-10 and C-string input)
        ld	hl,#rec_buf + REC_FNAME
        ld	de,(load_fname)         ; de = user filename pointer
        ld	c,#10
        ld	b,#0                    ; b=1 once trailing-space compare mode is active
.ld_cmp:
        ld	a,b
        or	a
        jr	nz,.ld_cmp_space
        ld	a,(de)
        or	a
        jr	nz,.ld_cmp_have
        ld	b,#1
.ld_cmp_space:
        ld	a,#' '
.ld_cmp_have:
        cp	(hl)                    ; compare user char vs record char
        jp	nz,.ld_next
        inc	hl
        ld	a,b
        or	a
        jr	nz,.ld_cmp_noinc
        inc	de
.ld_cmp_noinc:
        dec	c
        jr	nz,.ld_cmp
        ;; filename matched — copy captured payload
        ld	a,(rec_buf + REC_LEN)    ; bc = chunk length
        ld	c,a
        ld	a,(rec_buf + REC_LEN + 1)
        ld	b,a
        ld	hl,(load_dest)           ; target = base + rec_num * 512
        ld	a,(rec_buf + REC_NUM)
        cp	#128
        jr	nc,.ld_next
        add	a,a
        ld	e,a
        ld	a,h
        add	a,e
        jr	c,.ld_next
        ld	h,a
        ld	de,#load_sec_buf         ; source payload start
        ld	a,b
        or	c
        jr	z,.ld_mark_found
.ld_copy:
        ld	a,(de)
        ld	(hl),a
        inc	de
        inc	hl
        dec	bc
        ld	a,b
        or	c
        jr	nz,.ld_copy
.ld_mark_found:
        ld	a,#1
        ld	(load_found),a
        jp	.ld_next
.ld_next:
        pop	bc                      ; restore sector counter
        inc	b
        ld	a,b
        cp	#254                    ; scanned enough sectors?
        jp	nz,.ld_scan
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
