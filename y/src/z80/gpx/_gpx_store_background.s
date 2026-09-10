        ;; _gpx_store_background.s
        ;;
        ;; Private ZX Spectrum save-under background capture.
        ;;
        ;; Inputs:
        ;;   HL = destination background bmp_t *
        ;;   B = y
        ;;   C = x
        ;;   D = visible width  (1..16)
        ;;   E = visible height (1..16)
        ;;
        ;; Saves the visible screen area into HL as a valid standard 1bpp
        ;; bmp_t with stride=2. Bytes/rows clipped off-screen remain zero
        ;; by clearing only the trailing rows after capture.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module _gpx_store_background
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_store_background
        .globl  __vid_rowaddr
        .globl  __vid_nextrow
        .globl  __gpx_ffshr

        .equ    BG_HEADER_SIZE,          5
        .equ    BG_PAYLOAD_SIZE,         32

        .equ    B_VISW,                  -1
        .equ    B_Y,                     -2
        .equ    B_SHIFT,                 -3
        .equ    B_XBYTE,                 -4
        .equ    B_SRCSPAN,               -5

        .area   _CODE

        ;; Clobbers:
        ;;   AF, BC, DE, BC', DE', HL'. Preserves HL, IX and IY.
__gpx_store_background:
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      hl,#-5
        add     hl,sp
        ld      sp,hl

        ld      B_VISW(ix),d
        ld      a,e
        exx
        ld      b,a                     ; visible rows
        ld      a,#16
        sub     b
        ld      c,a                     ; trailing rows to clear
        exx
        ld      B_Y(ix),b               ; parks y until xbyte is known

        ld      a,c
        and     #0x07
        ld      B_SHIFT(ix),a

        ld      a,c
        rrca
        rrca
        rrca
        and     #0x1f
        ld      B_XBYTE(ix),a

        ld      l,2(ix)
        ld      h,3(ix)
        ld      de,#BG_HEADER_SIZE
        add     hl,de
        ;; Alternate HL is the payload pointer, DE the masks and BC the
        ;; visible/trailing row counts. Only A crosses banks
        ;; when storing pixels; the video helpers leave this bank alone.
        push    hl
        exx
        pop     hl
        exx

        ;; Width, source byte count and right coverage never change between
        ;; rows. Derive both output masks once, including clipped widths.
        ld      a,B_VISW(ix)
        add     a,B_SHIFT(ix)
        ld      B_SRCSPAN(ix),a         ; 1..23 source bits to gather
        ld      a,B_VISW(ix)
        and     #7
        ld      a,#0xff
        jr      z,.gsb_mask_ready
        ld      a,B_VISW(ix)
        and     #7
        call    __gpx_ffshr
        cpl
.gsb_mask_ready:
        ld      d,a                     ; partial final byte, or FF at 8/16
        ld      e,#0
        ld      a,B_VISW(ix)
        cp      #9
        jr      c,.gsb_masks_store
        ld      e,d
        ld      d,#0xff
.gsb_masks_store:
        push    de
        exx
        pop     de                      ; fixed output masks
        exx

        ;; First row address, derived once; later rows are one __vid_nextrow
        ;; away. The row base low byte is a multiple of 0x20 and xbyte is
        ;; 0..31, so the add cannot carry.
        ld      b,B_Y(ix)               ; y, parked at entry
        call    __vid_rowaddr
        ld      a,B_XBYTE(ix)
        add     a,l
        ld      l,a

.gsb_row_loop:
        push    hl                      ; keep row base while gathering bytes
        ld      d,(hl)
        xor     a
        ld      e,a
        ld      c,a

        ld      b,B_SHIFT(ix)
        ld      a,B_SRCSPAN(ix)
        cp      #9
        jr      c,.gsb_src_ready
        inc     hl
        ld      e,(hl)
        cp      #17
        jr      c,.gsb_src_ready
        inc     hl
        ld      c,(hl)
.gsb_src_ready:

        ;; B still holds B_SHIFT (set above, untouched through src gather).
        ld      a,b
        or      a
        ld      a,d
        jr      z,.gsb_shift_done
.gsb_shift_loop:
        sla     c                       ; shifts a zero in and sets carry
        rl      e
        rla
        djnz    .gsb_shift_loop
.gsb_shift_done:

        exx
        and     d
        ld      (hl),a
        inc     hl
        exx
        ld      a,e
        exx
        and     e
        ld      (hl),a
        inc     hl
        dec     b
        exx

        pop     hl
        jr      z,.gsb_clear_tail
        call    __vid_nextrow
        jr      .gsb_row_loop

        ;; Captured rows already contain zero padding. Clear only the
        ;; unused rows, avoiding a full 32-byte clear before every capture.
.gsb_clear_tail:
        exx
        ld      b,c
        ld      a,b
        or      a
        jr      z,.gsb_done
        xor     a
.gsb_clear_loop:
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        djnz    .gsb_clear_loop
.gsb_done:
        ld      sp,ix
        pop     ix
        pop     hl
        ret
