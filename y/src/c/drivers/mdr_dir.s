        ;; mdr_dir.s
        ;;
        ;; ZX Spectrum Microdrive driver: directory listing entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_dir

        .globl  __mdr_select_drive
        .globl  __mdr_start_motor
        .globl  __mdr_read_hdr_rec
        .globl  __mdr_rec_len_valid
        .globl  __mdr_stop_motor

        .equ    MDR_FILE_SECS, 11
        .equ    MDR_FILE_SZ, 14
        .equ    REC_FLAG, 0
        .equ    REC_LEN, 2
        .equ    REC_FNAME, 4

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; _mdr_dir
        ;; Dispatch strategy:
        ;;   scan up to 254 sectors, aggregate records by 10-char filename.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_dir(uint8_t drive, mdr_file_t *files, uint8_t max)
        ;;
        ;; Arguments:
        ;;   A  = drive number (1-8)
        ;;   DE = pointer to mdr_file_t result array
        ;;   stack: max
_mdr_dir::
        ld	c,a                     ; preserve drive in C
        ;; fixed max (ysh buffer): avoid stack-arg ABI ambiguity here
        ld	b,#32
        ex	de,hl                   ; HL = files
        call	_enter_critical_section
        ld	(dir_buf_ptr),hl        ; save result buffer pointer
        xor	a
        ld	(dir_file_cnt),a        ; file count = 0
        ld	a,c                     ; restore drive
        call	__mdr_select_drive
        call	__mdr_start_motor
        ld	b,#0                    ; sector counter
.dir_scan:
        push	bc
        ;; read header+record from current stream position
        call	__mdr_read_hdr_rec
        jp	c,.dir_next
.dir_ok_rec:
        ;; skip free sectors (flag = 0)
        ld	a,(rec_buf + REC_FLAG)
        or	a
        jr	z,.dir_next
        ;; reject impossible record lengths (valid: 1..512)
        call	__mdr_rec_len_valid
        jr	c,.dir_next
        ;; skip blank filenames (space in first char)
        ld	a,(rec_buf + REC_FNAME)
        cp	#' '
        jr	z,.dir_next
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
        jr	nc,.dir_next
        ld	b,a                     ; B = current count
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
        pop	bc                      ; restore sector counter
        inc	b
        ld	a,b
        cp	#254                    ; scanned enough sectors?
        jp	nz,.dir_scan
        call	__mdr_stop_motor
.dir_done:
        ld	a,(dir_file_cnt)
        ld	l,a                     ; return count in L
        ;; sdcccall(1): drop stacked max (1 byte)
        pop	bc                      ; return address
        inc	sp
        push	bc
        call	_leave_critical_section
        ret
