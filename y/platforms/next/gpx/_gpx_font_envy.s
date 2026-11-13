        ;; _gpx_font_envy.s
        ;;
        ;; Decoded font blob (single-label layout).
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-30   TS

        .module _gpx_font_envy
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_font_envy

        .area   _CODE

_gpx_font_envy::
        ;; metadata (8 bytes)
        .db     0x01                    ; [0] flags: bit0=proportional, bit1=offset-endian
        .db     0x20                    ; [1] first_ascii
        .db     0x7F                    ; [2] last_ascii
        .db     0x05                    ; [3] empty_width
        .db     0x08                    ; [4] max_glyph_width
        .db     0x08                    ; [5] glyph_height
        .db     0x01                    ; [6] advance
        .db     0x01                    ; [7] descent

        ;; pointer table (96 uint16_t offsets, relative to font base)
        .dw     0x0000, 0x00C8, 0x00D5, 0x00E2, 0x00EF, 0x00FC
        ;; 0x20..0x25
        .dw     0x0109, 0x0116, 0x0123, 0x0130, 0x013D, 0x014A
        ;; 0x26..0x2B
        .dw     0x0157, 0x0164, 0x0171, 0x017E, 0x018B, 0x0198
        ;; 0x2C..0x31
        .dw     0x01A5, 0x01B2, 0x01BF, 0x01CC, 0x01D9, 0x01E6
        ;; 0x32..0x37
        .dw     0x01F3, 0x0200, 0x020D, 0x021A, 0x0227, 0x0234
        ;; 0x38..0x3D
        .dw     0x0241, 0x024E, 0x025B, 0x0268, 0x0275, 0x0282
        ;; 0x3E..0x43
        .dw     0x028F, 0x029C, 0x02A9, 0x02B6, 0x02C3, 0x02D0
        ;; 0x44..0x49
        .dw     0x02DD, 0x02EA, 0x02F7, 0x0304, 0x0311, 0x031E
        ;; 0x4A..0x4F
        .dw     0x032B, 0x0338, 0x0345, 0x0352, 0x035F, 0x036C
        ;; 0x50..0x55
        .dw     0x0379, 0x0386, 0x0393, 0x03A0, 0x03AD, 0x03BA
        ;; 0x56..0x5B
        .dw     0x03C7, 0x03D4, 0x03E1, 0x03EE, 0x03FB, 0x0408
        ;; 0x5C..0x61
        .dw     0x0415, 0x0422, 0x042F, 0x043C, 0x0449, 0x0456
        ;; 0x62..0x67
        .dw     0x0463, 0x0470, 0x047D, 0x048A, 0x0497, 0x04A4
        ;; 0x68..0x6D
        .dw     0x04B1, 0x04BE, 0x04CB, 0x04D8, 0x04E5, 0x04F2
        ;; 0x6E..0x73
        .dw     0x04FF, 0x050C, 0x0519, 0x0526, 0x0533, 0x0540
        ;; 0x74..0x79
        .dw     0x054D, 0x055A, 0x0567, 0x0574, 0x0581, 0x058E
        ;; 0x7A..0x7F
        ;; Zero offsets use header empty_width, including ASCII space.
        ;; No bitmap or per-character width is stored for the space.

        ;; glyph 0x21 '!'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x01                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x22 '"'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x03                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10100000              ; row 0
        .db     0b10100000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b00000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x23 '#'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b01010000              ; row 1
        .db     0b11111000              ; row 2
        .db     0b01010000              ; row 3
        .db     0b11111000              ; row 4
        .db     0b01010000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x24 '$'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00100000              ; row 0
        .db     0b01111000              ; row 1
        .db     0b10100000              ; row 2
        .db     0b01110000              ; row 3
        .db     0b00101000              ; row 4
        .db     0b11110000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x25 '%'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11000000              ; row 0
        .db     0b11001000              ; row 1
        .db     0b00010000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b10011000              ; row 5
        .db     0b00011000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x26 '&'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01100000              ; row 0
        .db     0b10010000              ; row 1
        .db     0b10010000              ; row 2
        .db     0b01111000              ; row 3
        .db     0b10010000              ; row 4
        .db     0b10010000              ; row 5
        .db     0b01111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x27 '\''
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x01                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b00000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x28 '('
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x03                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00100000              ; row 0
        .db     0b01000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b01000000              ; row 6
        .db     0b00100000              ; row 7

        ;; glyph 0x29 ')'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x03                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b01000000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b01000000              ; row 6
        .db     0b10000000              ; row 7

        ;; glyph 0x2A '*'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b01010000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b11111000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b01010000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x2B '+'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00100000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b11111000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x2C ','
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x02                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b00000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b01000000              ; row 6
        .db     0b10000000              ; row 7

        ;; glyph 0x2D '-'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b11111000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x2E '.'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x01                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b00000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x2F '/'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x04                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00010000              ; row 0
        .db     0b00010000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b10000000              ; row 7

        ;; glyph 0x30 '0'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10011000              ; row 2
        .db     0b10101000              ; row 3
        .db     0b11001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x31 '1'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00100000              ; row 0
        .db     0b01100000              ; row 1
        .db     0b10100000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x32 '2'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b00001000              ; row 2
        .db     0b00110000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x33 '3'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b00001000              ; row 2
        .db     0b00110000              ; row 3
        .db     0b00001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x34 '4'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00010000              ; row 0
        .db     0b00110000              ; row 1
        .db     0b01010000              ; row 2
        .db     0b10010000              ; row 3
        .db     0b11111000              ; row 4
        .db     0b00010000              ; row 5
        .db     0b00010000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x35 '5'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11111000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b00001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x36 '6'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00110000              ; row 0
        .db     0b01000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x37 '7'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11111000              ; row 0
        .db     0b00001000              ; row 1
        .db     0b00001000              ; row 2
        .db     0b00010000              ; row 3
        .db     0b00010000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x38 '8'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b01110000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x39 '9'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b01111000              ; row 3
        .db     0b00001000              ; row 4
        .db     0b00010000              ; row 5
        .db     0b01100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x3A ':'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x01                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x3B ';'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x02                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b01000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b01000000              ; row 6
        .db     0b10000000              ; row 7

        ;; glyph 0x3C '<'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x04                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00010000              ; row 0
        .db     0b00100000              ; row 1
        .db     0b01000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00010000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x3D '='
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b11111000              ; row 2
        .db     0b00000000              ; row 3
        .db     0b11111000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x3E '>'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x04                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b01000000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b00010000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x3F '?'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b00001000              ; row 2
        .db     0b00010000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x40 '@'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00110000              ; row 0
        .db     0b01001000              ; row 1
        .db     0b10011000              ; row 2
        .db     0b10101000              ; row 3
        .db     0b10011000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b00111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x41 'A'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b11111000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x42 'B'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b11110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x43 'C'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x44 'D'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11100000              ; row 0
        .db     0b10010000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10010000              ; row 5
        .db     0b11100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x45 'E'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11111000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x46 'F'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11111000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x47 'G'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b10011000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x48 'H'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b11111000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x49 'I'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11111000              ; row 0
        .db     0b00100000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x4A 'J'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00001000              ; row 0
        .db     0b00001000              ; row 1
        .db     0b00001000              ; row 2
        .db     0b00001000              ; row 3
        .db     0b00001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x4B 'K'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10010000              ; row 1
        .db     0b10100000              ; row 2
        .db     0b11000000              ; row 3
        .db     0b10100000              ; row 4
        .db     0b10010000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x4C 'L'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x4D 'M'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b11011000              ; row 2
        .db     0b11011000              ; row 3
        .db     0b10101000              ; row 4
        .db     0b10101000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x4E 'N'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b11001000              ; row 2
        .db     0b10101000              ; row 3
        .db     0b10011000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x4F 'O'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x50 'P'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x51 'Q'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10101000              ; row 4
        .db     0b10010000              ; row 5
        .db     0b01101000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x52 'R'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11110000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b10100000              ; row 4
        .db     0b10010000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x53 'S'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01111000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b01110000              ; row 3
        .db     0b00001000              ; row 4
        .db     0b00001000              ; row 5
        .db     0b11110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x54 'T'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11111000              ; row 0
        .db     0b00100000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x55 'U'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x56 'V'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b01010000              ; row 3
        .db     0b01010000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x57 'W'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b10101000              ; row 2
        .db     0b10101000              ; row 3
        .db     0b11011000              ; row 4
        .db     0b11011000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x58 'X'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b01010000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b01010000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x59 'Y'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10001000              ; row 0
        .db     0b10001000              ; row 1
        .db     0b01010000              ; row 2
        .db     0b01010000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x5A 'Z'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11111000              ; row 0
        .db     0b00001000              ; row 1
        .db     0b00010000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x5B '['
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x03                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11100000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b11100000              ; row 7

        ;; glyph 0x5C '\\'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x04                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b01000000              ; row 2
        .db     0b01000000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00010000              ; row 6
        .db     0b00010000              ; row 7

        ;; glyph 0x5D ']'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x03                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11100000              ; row 0
        .db     0b00100000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b11100000              ; row 7

        ;; glyph 0x5E '^'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00100000              ; row 0
        .db     0b01010000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b00000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x5F '_'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x08                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b00000000              ; row 2
        .db     0b00000000              ; row 3
        .db     0b00000000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b11111111              ; row 7

        ;; glyph 0x60 '`'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00110000              ; row 0
        .db     0b01001000              ; row 1
        .db     0b01000000              ; row 2
        .db     0b11110000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x61 'a'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01111000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10011000              ; row 5
        .db     0b01101000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x62 'b'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b11110000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b11110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x63 'c'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01111000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b01111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x64 'd'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00001000              ; row 0
        .db     0b00001000              ; row 1
        .db     0b01111000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x65 'e'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01110000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b11111000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b01111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x66 'f'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00110000              ; row 0
        .db     0b01001000              ; row 1
        .db     0b01000000              ; row 2
        .db     0b11100000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b01000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x67 'g'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01111000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b01111000              ; row 5
        .db     0b00001000              ; row 6
        .db     0b01110000              ; row 7

        ;; glyph 0x68 'h'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b11110000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x69 'i'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x02                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b01000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b11000000              ; row 2
        .db     0b01000000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b01000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x6A 'j'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x03                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00100000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01100000              ; row 2
        .db     0b00100000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b11000000              ; row 7

        ;; glyph 0x6B 'k'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10010000              ; row 3
        .db     0b11100000              ; row 4
        .db     0b10010000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x6C 'l'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x03                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11000000              ; row 0
        .db     0b01000000              ; row 1
        .db     0b01000000              ; row 2
        .db     0b01000000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b11100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x6D 'm'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b11010000              ; row 2
        .db     0b10101000              ; row 3
        .db     0b10101000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x6E 'n'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b10110000              ; row 2
        .db     0b11001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x6F 'o'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01110000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x70 'p'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b11110000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b11110000              ; row 6
        .db     0b10000000              ; row 7

        ;; glyph 0x71 'q'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01111000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10001000              ; row 5
        .db     0b01111000              ; row 6
        .db     0b00001000              ; row 7

        ;; glyph 0x72 'r'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b10110000              ; row 2
        .db     0b11001000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x73 's'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01111000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b01110000              ; row 4
        .db     0b00001000              ; row 5
        .db     0b11110000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x74 't'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b01000000              ; row 1
        .db     0b11110000              ; row 2
        .db     0b01000000              ; row 3
        .db     0b01000000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b00111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x75 'u'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b10011000              ; row 5
        .db     0b01101000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x76 'v'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b01010000              ; row 4
        .db     0b01010000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x77 'w'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10101000              ; row 3
        .db     0b10101000              ; row 4
        .db     0b10101000              ; row 5
        .db     0b01010000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x78 'x'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b01010000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b01010000              ; row 5
        .db     0b10001000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x79 'y'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b10001000              ; row 2
        .db     0b10001000              ; row 3
        .db     0b10001000              ; row 4
        .db     0b01111000              ; row 5
        .db     0b00001000              ; row 6
        .db     0b01110000              ; row 7

        ;; glyph 0x7A 'z'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b11111000              ; row 2
        .db     0b00010000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b01000000              ; row 5
        .db     0b11111000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x7B '{'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00011000              ; row 0
        .db     0b00100000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b11000000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b00011000              ; row 7

        ;; glyph 0x7C '|'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x01                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b10000000              ; row 0
        .db     0b10000000              ; row 1
        .db     0b10000000              ; row 2
        .db     0b10000000              ; row 3
        .db     0b10000000              ; row 4
        .db     0b10000000              ; row 5
        .db     0b10000000              ; row 6
        .db     0b10000000              ; row 7

        ;; glyph 0x7D '}'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b11000000              ; row 0
        .db     0b00100000              ; row 1
        .db     0b00100000              ; row 2
        .db     0b00011000              ; row 3
        .db     0b00100000              ; row 4
        .db     0b00100000              ; row 5
        .db     0b00100000              ; row 6
        .db     0b11000000              ; row 7

        ;; glyph 0x7E '~'
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x05                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00000000              ; row 0
        .db     0b00000000              ; row 1
        .db     0b01001000              ; row 2
        .db     0b10101000              ; row 3
        .db     0b10010000              ; row 4
        .db     0b00000000              ; row 5
        .db     0b00000000              ; row 6
        .db     0b00000000              ; row 7

        ;; glyph 0x7F DEL
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x06                    ; width
        .db     0x08                    ; height
        .dw     0x0008                  ; bitmap byte size
        .db     0b00110000              ; row 0
        .db     0b01001000              ; row 1
        .db     0b10110100              ; row 2
        .db     0b11000100              ; row 3
        .db     0b11000100              ; row 4
        .db     0b10110100              ; row 5
        .db     0b01001000              ; row 6
        .db     0b00110000              ; row 7
