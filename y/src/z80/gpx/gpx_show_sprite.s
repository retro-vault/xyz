        ;; gpx_show_sprite.s
        ;;
        ;; ZX Spectrum save-under sprite show path.
        ;;
        ;; Policy:
        ;;   - accepts standard 1bpp and masked 1bpp bmp_t payloads
        ;;   - limited to 16x16 sprites
        ;;   - x/y must already be on-screen (no top/left clipping)
        ;;   - only right/bottom clipping is applied
        ;;   - saved background is stored as a valid standard 1bpp bmp_t
        ;;     with fixed stride=2 and zero-filled clipped bytes
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_show_sprite
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_show_sprite
        .globl  __gpx_store_background
        .globl  __gpx_sprite_blit_raw
        .globl  _gpx_draw_bmp
        .globl  __vid_rowaddr

        .equ    BMP_SIG_ENC_MASK,        0xF0
        .equ    BMP_SIG_1BPP,            0x00
        .equ    BMP_SIG_1BPP_MASK,       0x10
        .equ    BMP_SIG_1BPP_STRIDE2,    0x01
        .equ    SCRHEIGHT,               192
        .equ    BG_HEADER_SIZE,          5
        .equ    BG_PAYLOAD_SIZE,         32

        .equ    SPR_X_LO,                0
        .equ    SPR_X_HI,                1
        .equ    SPR_Y_LO,                2
        .equ    SPR_Y_HI,                3
        .equ    SPR_BMP_LO,              4
        .equ    SPR_BMP_HI,              5
        .equ    SPR_BG_LO,               6
        .equ    SPR_BG_HI,               7
        .equ    SPR_CLIP_LO,             8
        .equ    SPR_CLIP_HI,             9

        ;; Sprite fields stay in IY; only bitmap dimensions need locals.
        .equ    S_W,                     -1
        .equ    S_H,                     -2
        .equ    S_VISW,                  -3
        .equ    S_VISH,                  -4

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_show_sprite
        ;; Save the pixels under the sprite into sprite->background, then
        ;; draw the sprite over them, so gpx_hide_sprite can put the
        ;; artwork back exactly.
        ;;
        ;; Signature:
        ;;   void gpx_show_sprite(gpx_t *gpx, sprite_t *sprite)
        ;;
        ;; Arguments:
        ;;   HL = gpx
        ;;   DE = sprite; background must point at writable storage of at
        ;;        least GPX_SPRITE_BG_SIZE bytes
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL. Preserves IX and IY.
        ;;
        ;; References:
        ;;   __gpx_store_background
        ;;   __gpx_sprite_blit_raw
_gpx_show_sprite::
        ld      a,d
        or      e
        ret     z

        push    iy
        push    de
        pop     iy                      ; sprite fields survive helper calls
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-4
        add     hl,sp
        ld      sp,hl

        ld      a,SPR_X_HI(iy)
        or      SPR_Y_HI(iy)
        jp      nz,.gs_done
        ld      a,SPR_Y_LO(iy)
        cp      #SCRHEIGHT
        jp      nc,.gs_done

        ld      a,SPR_BG_LO(iy)
        or      SPR_BG_HI(iy)
        jp      z,.gs_done
        ld      l,SPR_BMP_LO(iy)
        ld      h,SPR_BMP_HI(iy)
        ld      a,h
        or      l
        jp      z,.gs_done

        ld      a,(hl)
        and     #BMP_SIG_ENC_MASK
        cp      #BMP_SIG_1BPP
        jr      z,.gs_sig_ok
        cp      #BMP_SIG_1BPP_MASK
        jp      nz,.gs_done

.gs_sig_ok:
        ld      a,(hl)
        and     #0x0F
        inc     a
        cp      #3
        jp      nc,.gs_done

        inc     hl
        ld      a,(hl)
        ld      S_W(ix),a
        or      a
        jp      z,.gs_done
        cp      #17
        jp      nc,.gs_done

        inc     hl
        ld      a,(hl)
        ld      S_H(ix),a
        or      a
        jp      z,.gs_done
        cp      #17
        jp      nc,.gs_done

        ;; visible width = min(w, 256 - x)
        ld      a,S_W(ix)
        dec     a
        ld      b,SPR_X_LO(iy)
        add     a,b
        jr      nc,.gs_no_right_clip
        ld      a,SPR_X_LO(iy)
        cpl
        inc     a
        jr      .gs_store_visw
.gs_no_right_clip:
        ld      a,S_W(ix)
.gs_store_visw:
        ld      S_VISW(ix),a

        ;; visible height = min(h, 192 - y)
        ld      a,S_H(ix)
        dec     a
        add     a,SPR_Y_LO(iy)
        cp      #SCRHEIGHT
        jr      c,.gs_no_bottom_clip
        ld      b,SPR_Y_LO(iy)
        ld      a,#SCRHEIGHT
        sub     b
        jr      .gs_store_vish
.gs_no_bottom_clip:
        ld      a,S_H(ix)
.gs_store_vish:
        ld      S_VISH(ix),a

        ;; background sprite header
        ld      l,SPR_BG_LO(iy)
        ld      h,SPR_BG_HI(iy)
        ld      (hl),#BMP_SIG_1BPP_STRIDE2
        inc     hl
        ld      a,S_W(ix)
        ld      (hl),a
        inc     hl
        ld      a,S_H(ix)
        ld      (hl),a
        inc     hl
        ld      a,S_H(ix)
        add     a,a
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a

        ld      l,SPR_BG_LO(iy)
        ld      h,SPR_BG_HI(iy)
        ld      b,SPR_Y_LO(iy)
        ld      c,SPR_X_LO(iy)
        ld      d,S_VISW(ix)
        ld      e,S_VISH(ix)
        call    __gpx_store_background

        ;; window rect set? draw through the clipping blitter instead
        ;; (capture above stays full-box, so hide heals clipped pixels)
        ld      a,SPR_CLIP_LO(iy)
        or      SPR_CLIP_HI(iy)
        jr      nz,.gs_clipped

        ld      l,SPR_BMP_LO(iy)
        ld      h,SPR_BMP_HI(iy)
        ld      b,SPR_Y_LO(iy)
        ld      c,SPR_X_LO(iy)
        xor     a                       ; standard bitmaps keep zero bits transparent
        call    __gpx_sprite_blit_raw
        jr      .gs_done

.gs_clipped:
        ld      l,SPR_CLIP_LO(iy)
        ld      h,SPR_CLIP_HI(iy)
        push    hl                      ; clip
        ld      l,SPR_BMP_LO(iy)
        ld      h,SPR_BMP_HI(iy)
        push    hl                      ; bitmap
        ld      l,SPR_Y_LO(iy)
        ld      h,#0
        push    hl                      ; y
        ld      e,SPR_X_LO(iy)
        ld      d,#0                    ; DE = x (gpx arg unused by draw_bmp)
        call    _gpx_draw_bmp

.gs_done:
        ld      sp,ix
        pop     ix
        pop     iy
        ret
