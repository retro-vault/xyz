        ;; _gpx_sprite_blit_raw.s
        ;;
        ;; Private ZX Spectrum raw sprite blitter.
        ;;
        ;; Inputs:
        ;;   A = standard bitmap mode
        ;;       0 => transparent zero bits
        ;;       1 => copy zero bits too
        ;;   B = y (0..191)
        ;;   C = x (0..255)
        ;;   HL = bmp_t *
        ;;
        ;; Supported formats:
        ;;   standard 1bpp, stride 1..2
        ;;   masked 1bpp,   stride 1..2
        ;;
        ;; Policy:
        ;;   - no top/left clipping
        ;;   - right/bottom clipping only
        ;;   - max size 16x16
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module _gpx_sprite_blit_raw
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_sprite_blit_raw
        .globl  __vid_rowaddr
        .globl  __vid_nextrow
        .globl  __gpx_ffshr

        .equ    BMP_SIG_ENC_MASK,        0xF0
        .equ    BMP_SIG_1BPP,            0x00
        .equ    BMP_SIG_1BPP_MASK,       0x10
        .equ    SCRHEIGHT,               192

        .equ    P_ROWADV,                -1
        .equ    P_STDMODE,               -2
        .equ    P_STRIDE,                -3
        .equ    P_HLEFT,                 -4
        .equ    P_Y,                     -5
        .equ    P_SHIFT,                 -6
        .equ    P_XBYTE,                 -7
        .equ    P_INS0,                  -8
        .equ    P_INS1,                  -9
        .equ    P_INS2,                  -10
        .equ    P_SPAN,                  -11
        .equ    P_MASKED,                -12
        .equ    P_VISW,                  -13
        .equ    P_AND0,                  -14
        .equ    P_AND1,                  -15
        .equ    P_AND2,                  -16

        .area   _CODE

        ;; Clobbers:
        ;;   AF, BC, DE, HL. Preserves IX and IY.
__gpx_sprite_blit_raw:
        ld      d,h
        ld      e,l
        push    iy                      ; preserve caller IY (pinned OR src ptr)
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      hl,#-16
        add     hl,sp
        ld      sp,hl

        ld      P_STDMODE(ix),a
        ld      P_Y(ix),b               ; parks y until xbyte is known
        ld      l,e
        ld      h,d

        ld      a,c
        and     #0x07
        ld      P_SHIFT(ix),a

        ld      a,c
        rrca
        rrca
        rrca
        and     #0x1f
        ld      P_XBYTE(ix),a

        ld      a,(hl)
        and     #BMP_SIG_ENC_MASK
        cp      #BMP_SIG_1BPP
        jr      z,.gbr_std
        cp      #BMP_SIG_1BPP_MASK
        jr      z,.gbr_masked
        jp      .gbr_done

.gbr_std:
        xor     a
        ld      P_MASKED(ix),a
        jr      .gbr_sig_done

.gbr_masked:
        ld      a,#1
        ld      P_MASKED(ix),a

.gbr_sig_done:
        ld      a,(hl)
        and     #0x0F
        inc     a
        cp      #3
        jp      nc,.gbr_done
        ld      P_STRIDE(ix),a

        inc     hl
        ld      a,(hl)
        or      a
        jp      z,.gbr_done
        cp      #17
        jp      nc,.gbr_done
        ld      d,a

        inc     hl
        ld      a,(hl)
        or      a
        jp      z,.gbr_done
        cp      #17
        jp      nc,.gbr_done
        ld      e,a

        ld      a,d
        dec     a
        add     a,c
        jr      nc,.gbr_no_right_clip
        ld      a,c
        cpl
        inc     a
        jr      .gbr_visw_done
.gbr_no_right_clip:
        ld      a,d
.gbr_visw_done:
        ld      P_VISW(ix),a

        ld      a,e
        dec     a
        add     a,b
        cp      #SCRHEIGHT
        jr      c,.gbr_no_bottom_clip
        ld      a,#SCRHEIGHT
        sub     b
        jr      .gbr_h_done
.gbr_no_bottom_clip:
        ld      a,e
.gbr_h_done:
        ld      P_HLEFT(ix),a
        or      a
        jp      z,.gbr_done

        ld      de,#3
        add     hl,de

        ;; IY points at the OR plane. The masked AND bytes are immediately
        ;; before it, so no second source pointer is needed.
        ld      e,P_STRIDE(ix)
        ld      d,#0
        ld      a,P_MASKED(ix)
        or      a
        jr      z,.gbr_ptrs_ready
        add     hl,de
        sla     e
.gbr_ptrs_ready:
        ld      P_ROWADV(ix),e
        push    hl
        pop     iy

        ;; left-keep = ~(0xff >> shift) (shift 0 -> keep nothing = 0)
        ld      a,P_SHIFT(ix)
        call    __gpx_ffshr
        cpl
        ld      d,a

        ld      a,P_SHIFT(ix)
        add     a,P_VISW(ix)
        ld      e,a
        add     a,#7
        rrca
        rrca
        rrca
        and     #0x1f
        ld      P_SPAN(ix),a

        ;; right-keep = 0xff >> ((shift+visw) & 7), 0 -> 0
        ld      a,e
        and     #0x07
        jr      z,.gbr_rk_store         ; A = 0 stands
        call    __gpx_ffshr
.gbr_rk_store:
        ld      e,a

        ld      a,P_SPAN(ix)
        cp      #1
        jr      nz,.gbr_span2_or3
        ld      a,d
        or      e
        cpl
        ld      P_INS0(ix),a
        jr      .gbr_setup_and

.gbr_span2_or3:
        ld      a,d
        cpl
        ld      P_INS0(ix),a

        ld      a,P_SPAN(ix)
        cp      #2
        jr      nz,.gbr_span3
        ld      a,e
        cpl
        ld      P_INS1(ix),a
        jr      .gbr_setup_and

.gbr_span3:
        ld      a,#0xFF
        ld      P_INS1(ix),a
        ld      a,e
        cpl
        ld      P_INS2(ix),a

.gbr_setup_and:
        ;; The insertion masks discard all padding. Within the visible
        ;; width a plain AND plane is uniformly FF (transparent) or 00
        ;; (copy), regardless of alignment.
        ld      a,P_STDMODE(ix)
        sub     #1
        sbc     a,a
        ld      P_AND0(ix),a
        ld      P_AND1(ix),a
        ld      P_AND2(ix),a

.gbr_first_row:
        ;; First destination row address, derived once. Every later row is one
        ;; __vid_nextrow away. The row base low byte is a multiple of 0x20 and
        ;; xbyte is 0..31, so the add cannot carry.
        ld      b,P_Y(ix)               ; y, parked at entry
        call    __vid_rowaddr
        ld      a,P_XBYTE(ix)
        add     a,l
        ld      l,a

.gbr_row_loop:
        push    hl                      ; destination row base

        ;; Gather the OR plane into B:C:L and the optional AND plane into
        ;; D:E:H. Both windows share one shift loop for masked sprites.
        ld      b,0(iy)
        ld      c,#0
        ld      d,-1(iy)
        ld      e,#0xff
        ld      a,P_STRIDE(ix)
        dec     a
        jr      z,.gbr_sources_ready
        ld      c,1(iy)
        ld      e,d
        ld      d,-2(iy)
.gbr_sources_ready:
        ld      hl,#0
        ld      a,P_MASKED(ix)
        or      a
        jr      z,.gbr_or_plane
        ld      a,P_SHIFT(ix)
        or      a
        jr      z,.gbr_and_shift_done
.gbr_mask_shift_loop:
        srl     d
        rr      e
        rr      h
        srl     b
        rr      c
        rr      l
        dec     a
        jr      nz,.gbr_mask_shift_loop
.gbr_and_shift_done:
        ld      P_AND0(ix),d
        ld      P_AND1(ix),e
        ld      P_AND2(ix),h
        jr      .gbr_shift_done
.gbr_or_plane:
        ld      a,P_SHIFT(ix)
        or      a
        jr      z,.gbr_shift_done
.gbr_or_shift_loop:
        srl     b
        rr      c
        rr      l
        dec     a
        jr      nz,.gbr_or_shift_loop
.gbr_shift_done:
        ld      d,l
        pop     hl
        push    hl                      ; retain base through composition

        ;; compose: new = (old & ANDn) | ORn, then merge under INSn via
        ;; old ^ ((old ^ new) & INSn)
        ld      a,(hl)
        ld      e,a
        and     P_AND0(ix)
        or      b
        xor     e
        and     P_INS0(ix)
        xor     e
        ld      (hl),a

        ld      a,P_SPAN(ix)
        cp      #1
        jr      z,.gbr_next_row

        inc     hl
        ld      a,(hl)
        ld      e,a
        and     P_AND1(ix)
        or      c
        xor     e
        and     P_INS1(ix)
        xor     e
        ld      (hl),a

        ld      a,P_SPAN(ix)
        cp      #2
        jr      z,.gbr_next_row

        inc     hl
        ld      a,(hl)
        ld      e,a
        and     P_AND2(ix)
        or      d
        xor     e
        and     P_INS2(ix)
        xor     e
        ld      (hl),a

.gbr_next_row:
        dec     P_HLEFT(ix)
        jr      z,.gbr_done

        ld      e,P_ROWADV(ix)
        ld      d,#0
        add     iy,de
        pop     hl
        call    __vid_nextrow

        jp      .gbr_row_loop

.gbr_done:
        ld      sp,ix
        pop     ix
        pop     iy                      ; restore caller IY
        ret
