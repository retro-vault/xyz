        ;; gpx_hide_sprite.s
        ;;
        ;; ZX Spectrum save-under sprite hide path.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-07-12   TS

        .module gpx_hide_sprite
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_hide_sprite
        .globl  __gpx_sprite_blit_raw

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_hide_sprite
        ;; Put back the pixels gpx_show_sprite saved into
        ;; sprite->background, restoring the artwork underneath exactly.
        ;;
        ;; Signature:
        ;;   void gpx_hide_sprite(gpx_t *gpx, sprite_t *sprite)
        ;;
        ;; Arguments:
        ;;   HL = gpx
        ;;   DE = sprite; x, y, bitmap and clip must be unchanged since
        ;;        the matching show call
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, IY
        ;;
        ;; References:
        ;;   __gpx_sprite_blit_raw
_gpx_hide_sprite::
        ld      a,d
        or      e
        ret     z

        ex      de,hl

        ld      c,(hl)                  ; x low
        inc     hl
        ld      a,(hl)                  ; x high
        or      a
        ret     nz

        inc     hl
        ld      b,(hl)                  ; y low
        inc     hl
        ld      a,(hl)                  ; y high
        or      a
        ret     nz

        ld      a,b
        cp      #192
        ret     nc

        inc     hl                      ; skip bitmap pointer low
        inc     hl                      ; skip bitmap pointer high
        inc     hl                      ; advance to background pointer low
        ld      e,(hl)                  ; background pointer
        inc     hl
        ld      d,(hl)
        ld      a,d
        or      e
        ret     z

        ex      de,hl
        ld      a,#1                    ; standard bitmap copy mode
        jp      __gpx_sprite_blit_raw   ; tail call
