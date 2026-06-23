        ;; mdr_load_slice.s
        ;;
        ;; ZX Spectrum Microdrive driver: partial file load entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_load_slice

        .globl  __mdr_select_drive
        .globl  __mdr_start_motor
        .globl  __mdr_read_hdr_rec
        .globl  __mdr_rec_len_valid
        .globl  __mdr_name_match10
        .globl  __mdr_read_byte
        .globl  __mdr_skip_payload
        .globl  __mdr_stop_motor

        .equ    REC_NUM, 1
        .equ    REC_LEN, 2
        .equ    REC_FNAME, 4

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; _mdr_load_slice
        ;; Dispatch strategy:
        ;;   scan all sectors, copy a requested byte window for matching files.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_load_slice(uint8_t drive, char *name, uint8_t *dest,
        ;;                          uint16_t offset, uint16_t len)
        ;;
        ;; Arguments:
        ;;   A  = drive number (1-8)
        ;;   DE = pointer to 10-char space-padded filename
        ;;   stack: dest, offset, len
_mdr_load_slice::
        push    iy
        ld      iy,#4
        add     iy,sp
        call    _enter_critical_section
        push    af                    ; preserve drive across setup
        ld      hl,#load_fname
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      l,0(iy)               ; dest lo
        ld      h,1(iy)               ; dest hi
        ld      (load_dest),hl
        ld      l,2(iy)               ; offset lo
        ld      h,3(iy)               ; offset hi
        ld      (slice_off),hl
        ld      e,4(iy)               ; len lo
        ld      d,5(iy)               ; len hi
        add     hl,de                 ; end = offset + len
        ld      (slice_end),hl
        xor     a
        ld      (load_found),a
        pop     af
        pop     iy
        call    __mdr_select_drive
        call    __mdr_start_motor
        ld      b,#0
.lds_scan:
        push    bc
        call    __mdr_read_hdr_rec
        jr      c,.lds_next
        call    __mdr_rec_len_valid
        jr      c,.lds_skip
        ld      hl,#rec_buf + REC_FNAME
        ld      de,(load_fname)
        call    __mdr_name_match10
        jr      c,.lds_skip
        ld      a,#1
        ld      (load_found),a
        ld      a,(rec_buf + REC_NUM)
        cp      #128
        jr      nc,.lds_skip
        ld      l,#0
        add     a,a
        ld      h,a                   ; HL = record base offset (rec_num * 512)
        ld      a,(rec_buf + REC_LEN)
        ld      c,a
        ld      a,(rec_buf + REC_LEN + 1)
        ld      b,a
        ld      (slice_cur),hl
        ld      a,b
        or      c
        jr      z,.lds_checksum
.lds_copy:
        call    __mdr_read_byte
        push    af
        ld      hl,(slice_cur)
        ld      de,(slice_off)
        or      a
        sbc     hl,de
        jr      c,.lds_no_store
        ld      hl,(slice_cur)
        ld      de,(slice_end)
        or      a
        sbc     hl,de
        jr      nc,.lds_no_store
        ld      hl,(slice_cur)
        ld      de,(slice_off)
        or      a
        sbc     hl,de
        ld      de,(load_dest)
        add     hl,de
        pop     af
        ld      (hl),a
        jr      .lds_advance
.lds_no_store:
        pop     af
.lds_advance:
        ld      hl,(slice_cur)
        inc     hl
        ld      (slice_cur),hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,.lds_copy
.lds_checksum:
        call    __mdr_read_byte
        jr      .lds_next
.lds_skip:
        call    __mdr_skip_payload
.lds_next:
        pop     bc
        inc     b
        ld      a,b
        cp      #254
        jp      nz,.lds_scan
        call    __mdr_stop_motor
        ld      a,(load_found)
        or      a
        jr      nz,.lds_ok
        ld      a,#1
        jr      .lds_ret
.lds_ok:
        xor     a
.lds_ret:
        ld      l,a
        pop     bc
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        push    bc
        call    _leave_critical_section
        ret
