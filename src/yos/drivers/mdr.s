        ;; mdr.s
        ;;
        ;; ZX Spectrum Microdrive driver: dir and load.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-29   tstih

        .module mdr

        .equ    MD_DATA,   0xe7         ; microdrive data port
        .equ    MD_CTRL,   0xef         ; microdrive control port

        ;; mdr_file_t field offsets (sizeof = 14)
        .equ    MDR_FILE_NAME, 0        ; char name[11]  — null-terminated
        .equ    MDR_FILE_SECS, 11       ; uint8_t sectors
        .equ    MDR_FILE_SIZE, 12       ; uint16_t size   — little-endian
        .equ    MDR_FILE_SZ,   14       ; sizeof(mdr_file_t)

        ;; md_record_t field offsets within rec_buf
        .equ    REC_FLAG,      0        ; uint8_t  flag
        .equ    REC_NUM,       1        ; uint8_t  record number
        .equ    REC_LEN,       2        ; uint16_t length (lo at +2, hi at +3)
        .equ    REC_FNAME,     4        ; char     filename[10]
        .equ    REC_CHK,      14        ; uint8_t  checksum of bytes 0..13

        .area   _CODE

; ============================================================
; Public functions
; ============================================================

        ;; extern uint8_t _mdr_dir(uint8_t drive, mdr_file_t *files, uint8_t max);
        ;; param:  A  = drive number (1-8)
        ;;         HL = pointer to mdr_file_t result array
        ;;         B  = maximum number of entries to fill
        ;; return: L  = number of unique files found
        ;; affects: AF, BC, DE, HL
        ;; notes:   scans up to 200 sectors; aggregates multi-sector files;
        ;;          uniqueness determined by exact 10-char filename match
_mdr_dir::
        push    af                      ; save drive (A used below)
        ld      (dir_buf_ptr),hl        ; save result buffer pointer
        ld      a,b
        ld      (dir_buf_max),a         ; save max entries
        xor     a
        ld      (dir_file_cnt),a        ; file count = 0
        pop     af                      ; A = drive
        call    __mdr_select_drive
        call    __mdr_start_motor
        ld      b,#0                    ; sector counter
.dir_scan:
        push    bc
        call    __mdr_wait_gap_sync     ; wait for sector boundary
        ;; skip preamble (~12 bytes)
        ld      c,#12
.dir_pream:
        call    __mdr_read_byte
        dec     c
        jr      nz,.dir_pream
        ;; read 15-byte sector header
        ld      hl,#header_buf
        ld      c,#15
.dir_hdr:
        call    __mdr_read_byte
        ld      (hl),a
        inc     hl
        dec     c
        jr      nz,.dir_hdr
        ;; read 15-byte record descriptor
        ld      hl,#rec_buf
        ld      c,#15
.dir_rec:
        call    __mdr_read_byte
        ld      (hl),a
        inc     hl
        dec     c
        jr      nz,.dir_rec
        ;; skip free sectors (flag = 0)
        ld      a,(rec_buf + REC_FLAG)
        or      a
        jr      z,.dir_next
        ;; skip blank filenames (space in first char)
        ld      a,(rec_buf + REC_FNAME)
        cp      #' '
        jr      z,.dir_next
        ;; search result array for a matching filename
        ld      hl,(dir_buf_ptr)        ; HL = first entry
        ld      a,(dir_file_cnt)
        ld      b,a                     ; B = entries to search
        or      a
        jr      z,.dir_not_found        ; no entries yet
.dir_search:
        push    bc                      ; save search counter
        push    hl                      ; save entry pointer
        ld      de,#rec_buf + REC_FNAME ; DE = filename in record
        ld      c,#10                   ; compare 10 chars
.dir_cmp:
        ld      a,(de)
        cp      (hl)
        jr      nz,.dir_cmp_no
        inc     hl
        inc     de
        dec     c
        jr      nz,.dir_cmp
        ;; match — HL is at entry+10; update sectors and size
        pop     hl                      ; HL = entry base
        pop     bc                      ; discard search counter
        ld      de,#MDR_FILE_SECS       ; advance to sectors field
        add     hl,de
        inc     (hl)                    ; sectors++
        inc     hl                      ; → size lo (offset 12)
        ld      de,#rec_buf + REC_LEN
        ld      a,(de)                  ; rec length lo
        add     a,(hl)                  ; + current size lo
        ld      (hl),a                  ; store new size lo
        inc     hl                      ; → size hi (offset 13)
        inc     de                      ; → rec_buf + REC_LEN + 1
        ld      a,(de)                  ; rec length hi
        adc     a,(hl)                  ; + current size hi + carry
        ld      (hl),a                  ; store new size hi
        jr      .dir_next
.dir_cmp_no:
        pop     hl                      ; HL = entry base
        pop     bc                      ; restore search counter
        ld      de,#MDR_FILE_SZ         ; advance to next entry
        add     hl,de
        djnz    .dir_search
        ;; filename not in array — add new entry if room
.dir_not_found:
        ld      a,(dir_file_cnt)        ; count - max: C set if room remains
        ld      b,a
        ld      a,(dir_buf_max)
        cp      b
        jr      z,.dir_next             ; full
        jr      c,.dir_next             ; overflow guard
        ;; HL points to the next free slot (from search loop or dir_buf_ptr)
        ld      de,#rec_buf + REC_FNAME ; copy 10-char filename
        ld      c,#10
.dir_copy:
        ld      a,(de)
        ld      (hl),a
        inc     hl
        inc     de
        dec     c
        jr      nz,.dir_copy
        ld      (hl),#0                 ; null terminator (offset 10)
        inc     hl                      ; → sectors (offset 11)
        ld      (hl),#1                 ; sectors = 1
        inc     hl                      ; → size lo (offset 12)
        ld      de,#rec_buf + REC_LEN
        ld      a,(de)
        ld      (hl),a                  ; size lo
        inc     hl                      ; → size hi (offset 13)
        inc     de
        ld      a,(de)
        ld      (hl),a                  ; size hi
        ld      hl,#dir_file_cnt
        inc     (hl)                    ; file count++
.dir_next:
        pop     bc                      ; restore sector counter
        inc     b
        ld      a,b
        cp      #200                    ; scanned enough sectors?
        jp      nz,.dir_scan
        call    __mdr_stop_motor
        ei
        ld      a,(dir_file_cnt)
        ld      l,a                     ; return count in L
        ret

        ;; extern uint8_t _mdr_load(uint8_t drive, char *name, uint8_t *dest);
        ;; param:  A  = drive number (1-8)
        ;;         HL = pointer to 10-char space-padded filename
        ;;         DE = destination address for 512-byte data block
        ;; return: A = 0 on success, 1 if file not found
        ;; affects: AF, BC, DE, HL
        ;; notes:   loads first matching sector only; extend for multi-sector files
_mdr_load::
        ld      (load_fname),hl         ; save filename pointer
        call    __mdr_select_drive
        call    __mdr_start_motor
        ld      b,#0                    ; sector counter
.ld_scan:
        push    bc                      ; [sp+2] sector counter
        push    de                      ; [sp+0] destination address
        call    __mdr_wait_gap_sync     ; wait for sector boundary
        ;; skip preamble (~12 bytes)
        ld      c,#12
.ld_pream:
        call    __mdr_read_byte
        dec     c
        jr      nz,.ld_pream
        ;; read 15-byte sector header (discard)
        ld      hl,#header_buf
        ld      c,#15
.ld_hdr:
        call    __mdr_read_byte
        ld      (hl),a
        inc     hl
        dec     c
        jr      nz,.ld_hdr
        ;; read 15-byte record descriptor
        ld      hl,#rec_buf
        ld      c,#15
.ld_rec:
        call    __mdr_read_byte
        ld      (hl),a
        inc     hl
        dec     c
        jr      nz,.ld_rec
        ;; compare 10-char filename at rec_buf+REC_FNAME against saved pointer
        ld      hl,#rec_buf + REC_FNAME
        ld      de,(load_fname)         ; de = user filename pointer
        ld      c,#10
.ld_cmp:
        ld      a,(de)
        cp      (hl)                    ; compare user char vs record char
        jr      nz,.ld_no_match
        inc     hl
        inc     de
        dec     c
        jr      nz,.ld_cmp
        ;; filename matched — read 512 data bytes to destination
        pop     de                      ; [sp+0] restore destination address
        pop     bc                      ; [sp+2] discard sector counter
        ld      b,#0                    ; first 256 bytes
.ld_data1:
        call    __mdr_read_byte
        ld      (de),a
        inc     de
        djnz    .ld_data1
        ld      b,#0                    ; second 256 bytes
.ld_data2:
        call    __mdr_read_byte
        ld      (de),a
        inc     de
        djnz    .ld_data2
        call    __mdr_read_byte         ; discard data checksum byte
        call    __mdr_stop_motor
        ei
        ld      l,#0                    ; return 0 = success
        ret
.ld_no_match:
        pop     de                      ; discard saved destination
        pop     bc                      ; restore sector counter
        inc     b
        ld      a,b
        cp      #200                    ; scanned enough sectors?
        jr      nz,.ld_scan
        call    __mdr_stop_motor
        ei
        ld      l,#1                    ; return 1 = file not found
        ret

        ;; extern uint8_t _mdr_detect_drives(void);
        ;; param:  (none)
        ;; return: A = number of detected drives (0-8)
        ;; affects: AF, BC, DE
        ;; notes:   tests each drive 1-8 by polling status bits 7-4 after
        ;;          motor start; any activity counts as drive present
_mdr_detect_drives::
        di
        ld      e,#0                    ; e = detected drive count
        ld      d,#1                    ; d = current drive under test
.det_loop:
        ld      a,d
        call    __mdr_select_drive      ; select drive d
        call    __mdr_start_motor
        ld      b,#100                  ; poll up to 100 times
.det_poll:
        ld      c,#MD_CTRL
        in      a,(c)
        and     #0xf0                   ; check bits 7-4 for any activity
        jr      nz,.det_present         ; any bit set → drive present
        call    __mdr_delay_10us
        djnz    .det_poll
        jr      .det_next               ; no response — not present
.det_present:
        inc     e                       ; count this drive
.det_next:
        call    __mdr_stop_motor
        inc     d
        ld      a,d
        cp      #9                      ; past drive 8?
        jr      nz,.det_loop
        ld      a,e                     ; return count in A
        ld      l,a                     ; also in L for C return value
        ei
        ret

        ;; extern uint8_t _mdr_save(uint8_t drive, char *name, uint8_t *src, uint16_t len);
        ;; param:  A  = drive number (1-8)
        ;;         HL = pointer to 10-char space-padded filename
        ;;         DE = source data address
        ;;         BC = data length (1-512 bytes)
        ;; return: L  = 0 on success, 1 if no free sector found
        ;; affects: AF, BC, DE, HL
        ;; notes:   writes to the first free sector found (single-sector file);
        ;;          a free sector has flag == 0 in its record descriptor
_mdr_save::
        push    af                      ; save drive
        ld      (save_name),hl          ; save filename pointer
        ld      (save_src),de           ; save source pointer
        ld      (save_len),bc           ; save data length
        pop     af                      ; A = drive
        call    __mdr_select_drive
        call    __mdr_start_motor
        ld      b,#0                    ; sector counter
.sav_scan:
        push    bc
        call    __mdr_wait_gap_sync     ; wait for sector boundary
        ;; skip preamble (~12 bytes)
        ld      c,#12
.sav_pream:
        call    __mdr_read_byte
        dec     c
        jr      nz,.sav_pream
        ;; read 15-byte sector header
        ld      hl,#header_buf
        ld      c,#15
.sav_hdr:
        call    __mdr_read_byte
        ld      (hl),a
        inc     hl
        dec     c
        jr      nz,.sav_hdr
        ;; read 15-byte record descriptor
        ld      hl,#rec_buf
        ld      c,#15
.sav_rec:
        call    __mdr_read_byte
        ld      (hl),a
        inc     hl
        dec     c
        jr      nz,.sav_rec
        ;; skip sectors in use (flag != 0)
        ld      a,(rec_buf + REC_FLAG)
        or      a
        jr      nz,.sav_next
        ;; free sector found — write header, record, data
        pop     bc                      ; discard sector counter
        call    __mdr_write_hdr         ; rewrite sector header
        call    __mdr_write_rec         ; build and write record descriptor
        call    __mdr_write_data        ; write data block + pad + checksum
        call    __mdr_stop_motor
        ei
        ld      l,#0                    ; return 0 = success
        ret
.sav_next:
        pop     bc                      ; restore sector counter
        inc     b
        ld      a,b
        cp      #200                    ; scanned enough sectors?
        jr      nz,.sav_scan
        call    __mdr_stop_motor
        ei
        ld      l,#1                    ; return 1 = no free sector
        ret

        ;; __mdr_write_hdr
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC, HL
        ;; notes:   writes a 15-byte sector header from header_buf
__mdr_write_hdr::
        ld      a,#0x30                 ; motor on + write mode + erase
        out     (#MD_CTRL),a
        call    __mdr_delay_10us
        ld      hl,#header_buf
        ld      b,#15
.wh_loop:
        ld      a,(hl)
        call    __mdr_write_byte
        inc     hl
        djnz    .wh_loop
        ret

        ;; __mdr_write_rec
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC, DE, HL
        ;; notes:   builds and writes a 15-byte record descriptor
__mdr_write_rec::
        ld      a,#0x30                 ; motor on + write mode + erase
        out     (#MD_CTRL),a
        ld      hl,#rec_buf
        ld      (hl),#1                 ; flag: sector in use
        inc     hl
        ld      (hl),#0                 ; single-record file => record #0
        inc     hl
        ld      bc,(save_len)
        ld      (hl),c                  ; length low
        inc     hl
        ld      (hl),b                  ; length high
        inc     hl
        ld      de,(save_name)          ; copy 10-char padded filename
        ld      b,#10
.wr_name:
        ld      a,(de)
        ld      (hl),a
        inc     hl
        inc     de
        djnz    .wr_name
        ld      hl,#rec_buf
        ld      b,#14
        call    __mdr_calc_checksum
        ld      (rec_buf + REC_CHK),a
        ld      hl,#rec_buf
        ld      b,#15
.wr_emit:
        ld      a,(hl)
        call    __mdr_write_byte
        inc     hl
        djnz    .wr_emit
        ret

        ;; __mdr_write_data
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC, DE, HL
        ;; notes:   writes len bytes from save_src, pads with zeros to 512,
        ;;          then writes checksum over the 512-byte data block
__mdr_write_data::
        ld      a,#0x30                 ; motor on + write mode + erase
        out     (#MD_CTRL),a
        ld      hl,(save_src)
        ld      bc,(save_len)
        ld      e,#0                    ; running checksum accumulator
.wd_copy:
        ld      a,b
        or      c
        jr      z,.wd_pad_prep
        ld      a,(hl)
        call    __mdr_write_byte
        add     a,e
        ld      e,a
        inc     hl
        dec     bc
        jr      .wd_copy
.wd_pad_prep:
        ld      hl,#512
        ld      bc,(save_len)
        xor     a
        sbc     hl,bc                   ; HL = bytes to pad
        jr      z,.wd_checksum
.wd_pad:
        xor     a
        call    __mdr_write_byte
        dec     hl
        ld      a,h
        or      l
        jr      nz,.wd_pad
.wd_checksum:
        ld      a,e
        and     #0xfe                   ; keep checksum out of 0xff
        call    __mdr_write_byte
        ret

        ;; __mdr_write_byte
        ;; param:  A = byte to write
        ;; return: (none)
        ;; affects: BC
        ;; notes:   caller must preconfigure MD_CTRL for write mode
__mdr_write_byte::
        push    bc
        ld      c,#MD_DATA
        out     (c),a
        call    __mdr_delay_short
        pop     bc
        ret

; ============================================================
; Helpers
; ============================================================

        ;; __mdr_select_drive
        ;; param:  A = drive number (1-8)
        ;; return: (none)
        ;; affects: AF, BC
        ;; notes:   clocks the COMMS shift register to select the target drive;
        ;;          disables interrupts for the duration, re-enables on return
__mdr_select_drive::
        di
        push    bc
        push    af
        ld      b,#8                    ; 8 clock pulses for full chain
        ld      c,a                     ; c = target drive (1-8 countdown)
        ld      a,#0
        out     (#MD_CTRL),a            ; deselect all
.sel_loop:
        ld      a,#0
        out     (#MD_CTRL),a
        call    __mdr_delay_10us
        dec     c
        jr      nz,.sel_data_zero       ; not our drive — keep data low
        ld      a,#4                    ; COMMS DATA = 1 for target drive
.sel_data_zero:
        out     (#MD_CTRL),a
        call    __mdr_delay_10us
        or      #8                      ; COMMS CLK high
        out     (#MD_CTRL),a
        call    __mdr_delay_10us
        and     #0xf7                   ; COMMS CLK low
        out     (#MD_CTRL),a
        djnz    .sel_loop
        ld      a,#0x20                 ; motor on + read mode
        out     (#MD_CTRL),a
        pop     af
        pop     bc
        ei
        ret

        ;; __mdr_start_motor
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF
        ;; notes:   sets motor-on + read mode (R/W=1)
__mdr_start_motor::
        ld      a,#0x20                 ; motor on + read mode
        out     (#MD_CTRL),a
        ret

        ;; __mdr_stop_motor
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF
        ;; notes:   cuts motor power and deselects all drives
__mdr_stop_motor::
        ld      a,#0                    ; all off
        out     (#MD_CTRL),a
        ret

        ;; __mdr_wait_gap_sync
        ;; param:  (none)
        ;; return: (none)
        ;; affects: AF, BC
        ;; notes:   polls the control port until both GAP (bit 5) and
        ;;          SYNC (bit 4) are set; blocks until sector boundary found
__mdr_wait_gap_sync::
        ld      bc,#MD_CTRL
.wgs_poll:
        in      a,(c)
        bit     5,a                     ; GAP flag?
        jr      z,.wgs_poll
        bit     4,a                     ; SYNC flag?
        jr      z,.wgs_poll
        ret

        ;; __mdr_read_byte
        ;; param:  (none)
        ;; return: A = byte read from data port
        ;; affects: AF, BC
        ;; notes:   waits for GAP+SYNC before reading; assumes motor running
__mdr_read_byte::
        call    __mdr_wait_gap_sync
        ld      bc,#MD_DATA
        in      a,(c)
        ret

        ;; __mdr_calc_checksum
        ;; param:  HL = pointer to data block
        ;;         B  = byte count
        ;; return: A  = checksum (sum of bytes, mod 255, result != 255)
        ;; affects: AF, HL, BC
        ;; notes:   value 0xff is avoided by masking with 0xfe
__mdr_calc_checksum::
        push    bc
        push    hl
        ld      a,#0                    ; accumulator = 0
.cs_loop:
        add     a,(hl)                  ; sum bytes
        inc     hl
        djnz    .cs_loop
        and     #0xfe                   ; avoid 0xff
        pop     hl
        pop     bc
        ret

        ;; __mdr_delay_10us
        ;; param:  (none)
        ;; return: (none)
        ;; affects: BC
        ;; notes:   ~10us at 3.5MHz — adjust loop count for real hardware
__mdr_delay_10us::
        push    bc
        ld      b,#6                    ; ~10us at 3.5MHz
.d10us:
        djnz    .d10us
        pop     bc
        ret

        ;; __mdr_delay_short
        ;; param:  (none)
        ;; return: (none)
        ;; affects: BC
        ;; notes:   short settling delay — adjust loop count for real hardware
__mdr_delay_short::
        push    bc
        ld      b,#12                   ; short settling delay
.dshrt:
        djnz    .dshrt
        pop     bc
        ret

        .area   _BSS

load_fname:
        .ds     2                       ; saved filename pointer (mdr_load)
dir_buf_ptr:
        .ds     2                       ; result buffer pointer (mdr_dir)
dir_buf_max:
        .ds     1                       ; max entries (mdr_dir)
dir_file_cnt:
        .ds     1                       ; entries filled (mdr_dir)
header_buf:
        .ds     15                      ; sector header scratch buffer
rec_buf:
        .ds     15                      ; record descriptor scratch buffer
save_name:
        .ds     2                       ; saved filename pointer (mdr_save)
save_src:
        .ds     2                       ; saved source pointer (mdr_save)
save_len:
        .ds     2                       ; saved byte length (mdr_save)
