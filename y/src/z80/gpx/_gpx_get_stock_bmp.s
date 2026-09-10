        ;; _gpx_get_stock_bmp.s
        ;;
        ;; Resolve stock bitmap id to cursor bitmap blob.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-29   TS

        .module _gpx_get_stock_bmp
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_get_stock_bmp
        .globl  _gpx_cur_classic
        .globl  _gpx_cur_std
        .globl  _gpx_cur_hourglass
        .globl  _gpx_cur_caret
        .globl  _gpx_cur_hand

        .equ    GPXSB_CURSOR_CLASSIC,   0x00
        .equ    GPXSB_CURSOR_STD,       0x01
        .equ    GPXSB_CURSOR_HOURGLASS, 0x02
        .equ    GPXSB_CURSOR_CARET,     0x03
        .equ    GPXSB_CURSOR_HAND,      0x04

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_get_stock_bmp
        ;; Built-in artwork by id, in the masked 1bpp raster format this
        ;; backend draws. Using these keeps a program portable, since the
        ;; payload format itself is machine-specific.
        ;;
        ;; Signature:
        ;;   bmp_t *gpx_get_stock_bmp(uint8_t which)
        ;;
        ;; Arguments:
        ;;   A = which, one of the GPXSB_* ids
        ;;
        ;; Return:
        ;;   DE = bmp_t*, or 0 when the id is not known here
        ;;
        ;; Clobbers:
        ;;   AF, DE
_gpx_get_stock_bmp::
        or      a
        jr      z,.classic
        dec     a
        jr      z,.std
        dec     a
        jr      z,.hourglass
        dec     a
        jr      z,.caret
        dec     a
        jr      z,.hand

        ld      de,#0x0000
        ret

.classic:
        ld      de,#_gpx_cur_classic
        ret
.std:
        ld      de,#_gpx_cur_std
        ret
.hourglass:
        ld      de,#_gpx_cur_hourglass
        ret
.caret:
        ld      de,#_gpx_cur_caret
        ret
.hand:
        ld      de,#_gpx_cur_hand
        ret
