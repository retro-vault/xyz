        ;; _gpx_cursors.s
        ;;
        ;; Mouse cursor bitmaps extracted from buddy/graphics/mousecur.s.
        ;;
        ;; FORMAT (masked bmp_t + hotspot trailer):
        ;;   header (5 bytes):
        ;;     signature = masked 1bpp + stride nibble (stride-1)
        ;;     width     = 8
        ;;     height    = 10
        ;;     size      = 20 (10 rows * 2 bytes)
        ;;   payload (20 bytes): per row = AND mask byte, OR bitmap byte
        ;;   trailer (1 byte): hotspot (high nibble = y offset, low nibble = x offset)
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-30   TS

        .module _gpx_cursors
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_cur_classic
        .globl  _gpx_cur_std
        .globl  _gpx_cur_hourglass
        .globl  _gpx_cur_caret
        .globl  _gpx_cur_hand

        .area   _CODE

_gpx_cur_classic::
        .db     0x10                    ; signature: BMP_ENC_1BPP_MASK, stride=1
        .db     0x08                    ; width
        .db     0x0A                    ; height
        .dw     0x0014                  ; bitmap byte size
        .db     0b00111111, 0b00000000  ; row 0: and, or
        .db     0b00011111, 0b01000000  ; row 1: and, or
        .db     0b00001111, 0b01100000  ; row 2: and, or
        .db     0b00000111, 0b01110000  ; row 3: and, or
        .db     0b00000011, 0b01111000  ; row 4: and, or
        .db     0b00000001, 0b01111100  ; row 5: and, or
        .db     0b00000011, 0b01110000  ; row 6: and, or
        .db     0b00000011, 0b01011000  ; row 7: and, or
        .db     0b00100011, 0b00001000  ; row 8: and, or
        .db     0b11110111, 0b00000000  ; row 9: and, or
        .db     0x11                    ; hotspot: y=1, x=1

_gpx_cur_std::
        .db     0x10                    ; signature: BMP_ENC_1BPP_MASK, stride=1
        .db     0x08                    ; width
        .db     0x0A                    ; height
        .dw     0x0014                  ; bitmap byte size
        .db     0b11111111, 0b11000000  ; row 0: and, or
        .db     0b00011111, 0b10100000  ; row 1: and, or
        .db     0b00001111, 0b10010000  ; row 2: and, or
        .db     0b00000111, 0b10001000  ; row 3: and, or
        .db     0b00000011, 0b10000100  ; row 4: and, or
        .db     0b00000001, 0b10000010  ; row 5: and, or
        .db     0b00000011, 0b10001100  ; row 6: and, or
        .db     0b00000011, 0b10100100  ; row 7: and, or
        .db     0b10000011, 0b01100100  ; row 8: and, or
        .db     0b11100111, 0b00011000  ; row 9: and, or
        .db     0x11                    ; hotspot: y=1, x=1

_gpx_cur_hourglass::
        .db     0x10                    ; signature: BMP_ENC_1BPP_MASK, stride=1
        .db     0x08                    ; width
        .db     0x0A                    ; height
        .dw     0x0014                  ; bitmap byte size
        .db     0b00000001, 0b11111110  ; row 0: and, or
        .db     0b10000011, 0b01000100  ; row 1: and, or
        .db     0b10000011, 0b01010100  ; row 2: and, or
        .db     0b11000111, 0b00101000  ; row 3: and, or
        .db     0b11101111, 0b00010000  ; row 4: and, or
        .db     0b11000111, 0b00101000  ; row 5: and, or
        .db     0b10000011, 0b01000100  ; row 6: and, or
        .db     0b10000011, 0b01010100  ; row 7: and, or
        .db     0b00000001, 0b11111110  ; row 8: and, or
        .db     0b11111111, 0b00000000  ; row 9: and, or
        .db     0x43                    ; hotspot: y=4, x=3

_gpx_cur_caret::
        .db     0x10                    ; signature: BMP_ENC_1BPP_MASK, stride=1
        .db     0x08                    ; width
        .db     0x0A                    ; height
        .dw     0x0014                  ; bitmap byte size
        .db     0b00100111, 0b11011000  ; row 0: and, or
        .db     0b00000111, 0b10101000  ; row 1: and, or
        .db     0b00000111, 0b11011000  ; row 2: and, or
        .db     0b10001111, 0b01010000  ; row 3: and, or
        .db     0b10001111, 0b01010000  ; row 4: and, or
        .db     0b10001111, 0b01010000  ; row 5: and, or
        .db     0b10001111, 0b01010000  ; row 6: and, or
        .db     0b00000111, 0b11011000  ; row 7: and, or
        .db     0b00000111, 0b10101000  ; row 8: and, or
        .db     0b00100111, 0b11011000  ; row 9: and, or
        .db     0x71                    ; hotspot: y=7, x=1

_gpx_cur_hand::
        .db     0x10                    ; signature: BMP_ENC_1BPP_MASK, stride=1
        .db     0x08                    ; width
        .db     0x0A                    ; height
        .dw     0x0014                  ; bitmap byte size
        .db     0b11011111, 0b00100000  ; row 0: and, or
        .db     0b10001111, 0b01010000  ; row 1: and, or
        .db     0b10000011, 0b01011100  ; row 2: and, or
        .db     0b10000001, 0b01010110  ; row 3: and, or
        .db     0b00000000, 0b11010101  ; row 4: and, or
        .db     0b00000000, 0b10010101  ; row 5: and, or
        .db     0b00000000, 0b10000001  ; row 6: and, or
        .db     0b00000000, 0b10000001  ; row 7: and, or
        .db     0b10000001, 0b01000010  ; row 8: and, or
        .db     0b11000011, 0b00111100  ; row 9: and, or
        .db     0x02                    ; hotspot: y=0, x=2
