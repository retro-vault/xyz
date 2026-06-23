        ;; mdr_format.s
        ;;
        ;; ZX Spectrum Microdrive driver: format entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_format

        .globl  __mdr_select_drive
        .globl  __mdr_start_motor
        .globl  __mdr_wait_gap_sync_long
        .globl  __mdr_write_hdr
        .globl  __mdr_write_byte
        .globl  __mdr_calc_checksum
        .globl  __mdr_stop_motor

        .equ    MD_CTRL, 0xef
        .equ    MD_CTL_WRITE_ERASE, 0xea

        .equ    HDR_FLAG, 0
        .equ    HDR_SECTOR, 1
        .equ    HDR_CART, 4
        .equ    HDR_CHK, 14

        .equ    REC_FLAG, 0
        .equ    REC_RECNUM, 1
        .equ    REC_LEN, 2
        .equ    REC_FNAME, 4
        .equ    REC_CHK, 14

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; _mdr_format
        ;; Dispatch strategy:
        ;;   stream-write 254 free sectors with regenerated headers.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_format(uint8_t drive, char *cart_name)
        ;;
        ;; Arguments:
        ;;   A  = drive number (1-8)
        ;;   DE = cartridge name (C string, padded to 10 chars)
_mdr_format::
        ex      de,hl                   ; HL = cart_name pointer
        call    _enter_critical_section
        push    af
        ld      (fmt_name),hl
        pop     af
        call    __mdr_select_drive
        call    __mdr_start_motor
        ld      a,#254
        ld      (fmt_sec_num),a
.fmt_loop:
        call    .fmt_make_header
        ;; block 1: preamble + header
        ld      a,#MD_CTL_WRITE_ERASE
        out     (#MD_CTRL),a
        call    .fmt_write_preamble
        call    __mdr_write_hdr
        call    __mdr_start_motor
        ;; block 2: preamble + free record + zero data
        ld      a,#MD_CTL_WRITE_ERASE
        out     (#MD_CTRL),a
        call    .fmt_write_preamble
        call    .fmt_write_free_record
        call    .fmt_write_zero_data
        call    __mdr_start_motor
        ld      hl,#fmt_sec_num
        dec     (hl)
        jr      nz,.fmt_loop

        ;; verify we can see GAP+SYNC after writing; this keeps absent drives failing.
        call    __mdr_start_motor
        call    __mdr_wait_gap_sync_long
        jr      c,.fmt_fail
        call    __mdr_stop_motor
        xor     a
        ld      l,a
        call    _leave_critical_section
        ret
.fmt_fail:
        call    __mdr_stop_motor
        ld      a,#1
        ld      l,a
        call    _leave_critical_section
        ret

        ;; ------------------------------------------------------------
        ;; .fmt_make_header
        ;; Rebuild 15-byte sector header with current sector number.
.fmt_make_header:
        ld      hl,#header_buf
        ld      (hl),#0x0f             ; standard IF1 header flag
        inc     hl
        ld      a,(fmt_sec_num)
        ld      (hl),a                 ; sector number (254..1)
        inc     hl
        xor     a
        ld      (hl),a                 ; unused[0]
        inc     hl
        ld      (hl),a                 ; unused[1]
        inc     hl
        ld      de,(fmt_name)
        ld      c,#10
        ld      b,#0                   ; trailing-space mode
.fmt_hname:
        ld      a,b
        or      a
        jr      nz,.fmt_hpad
        ld      a,(de)
        or      a
        jr      nz,.fmt_hhave
        ld      b,#1
.fmt_hpad:
        ld      a,#' '
        jr      .fmt_hstore
.fmt_hhave:
        inc     de
.fmt_hstore:
        ld      (hl),a
        inc     hl
        dec     c
        jr      nz,.fmt_hname
        ld      hl,#header_buf
        ld      b,#14
        call    __mdr_calc_checksum
        ld      (header_buf + HDR_CHK),a
        ret

        ;; ------------------------------------------------------------
        ;; .fmt_write_free_record
        ;; Emit one free record descriptor (15 bytes).
.fmt_write_free_record:
        ld      hl,#rec_buf
        ld      (hl),#0                ; free flag
        inc     hl
        ld      (hl),#0                ; rec num
        inc     hl
        ld      (hl),#0                ; len lo
        inc     hl
        ld      (hl),#0                ; len hi
        inc     hl
        ld      b,#10
        ld      a,#' '
.fmt_rname:
        ld      (hl),a
        inc     hl
        djnz    .fmt_rname
        ld      hl,#rec_buf
        ld      b,#14
        call    __mdr_calc_checksum
        ld      (rec_buf + REC_CHK),a
        ld      hl,#rec_buf
        ld      b,#15
.fmt_remit:
        ld      a,(hl)
        call    __mdr_write_byte
        inc     hl
        djnz    .fmt_remit
        ret

        ;; ------------------------------------------------------------
        ;; .fmt_write_zero_data
        ;; Emit 512 zero bytes + data checksum 0.
.fmt_write_zero_data:
        xor     a
        ld      b,#0
.fmt_z1:
        call    __mdr_write_byte
        djnz    .fmt_z1
        ld      b,#0
.fmt_z2:
        call    __mdr_write_byte
        djnz    .fmt_z2
        xor     a
        call    __mdr_write_byte
        ret

        ;; ------------------------------------------------------------
        ;; .fmt_write_preamble
        ;; Emit IF1 preamble: 10x00 then 2xFF.
.fmt_write_preamble:
        ld      b,#10
.fmt_p0:
        xor     a
        call    __mdr_write_byte
        djnz    .fmt_p0
        ld      b,#2
.fmt_pff:
        ld      a,#0xff
        call    __mdr_write_byte
        djnz    .fmt_pff
        ret

        .area   _BSS
fmt_name::
        .ds     2
fmt_sec_num::
        .ds     1
