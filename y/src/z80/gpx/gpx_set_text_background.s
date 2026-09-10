        ;; gpx_set_text_background.s
        ;;
        ;; ZX Spectrum text background policy setter.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-26   TS

        .module gpx_set_text_background
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_set_text_background

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_set_text_background
        ;; Select opaque (0) or transparent (1) text cell rendering.
        ;;
        ;; Signature:
        ;;   void gpx_set_text_background(gpx_t *gpx, textbg background)
        ;;
        ;; Arguments:
        ;;   HL = gpx
        ;;   stack: background
        ;;
        ;; Clobbers:
        ;;   AF, B, DE, HL
_gpx_set_text_background::
        pop     de                      ; return address
        dec     sp
        pop     af                      ; A = background, F = discarded ret high
        push    de                      ; sole stack argument already consumed
        ld      b,a
        ld      a,h
        or      l
        ret     z
        ld      a,b
        and     #0x01
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        ld      (hl),a
        ret
