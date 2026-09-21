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
        .globl  _gpx_cur_resize

        .equ    GPXSB_CURSOR_CLASSIC,   0x00
        .equ    GPXSB_CURSOR_STD,       0x01
        .equ    GPXSB_CURSOR_HOURGLASS, 0x02
        .equ    GPXSB_CURSOR_CARET,     0x03
        .equ    GPXSB_CURSOR_HAND,      0x04
        .equ    GPXSB_CURSOR_RESIZE,    0x05

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
        cp      #6
        jr      nc,.unknown
        ld      l,a
        ld      h,#0
        add     hl,hl
        ld      de,#.table
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ret
.unknown:
        ld      de,#0x0000
        ret
.table:
        .dw     _gpx_cur_classic
        .dw     _gpx_cur_std
        .dw     _gpx_cur_hourglass
        .dw     _gpx_cur_caret
        .dw     _gpx_cur_hand
        .dw     _gpx_cur_resize
