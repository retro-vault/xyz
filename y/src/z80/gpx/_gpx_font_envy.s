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
        .dw     0x00C8, 0x00CD, 0x00DA, 0x00E7, 0x00F4, 0x0101
        ;; 0x20..0x25
        .dw     0x010E, 0x011B, 0x0128, 0x0135, 0x0142, 0x014F
        ;; 0x26..0x2B
        .dw     0x015C, 0x0169, 0x0176, 0x0183, 0x0190, 0x019D
        ;; 0x2C..0x31
        .dw     0x01AA, 0x01B7, 0x01C4, 0x01D1, 0x01DE, 0x01EB
        ;; 0x32..0x37
        .dw     0x01F8, 0x0205, 0x0212, 0x021F, 0x022C, 0x0239
        ;; 0x38..0x3D
        .dw     0x0246, 0x0253, 0x0260, 0x026D, 0x027A, 0x0287
        ;; 0x3E..0x43
        .dw     0x0294, 0x02A1, 0x02AE, 0x02BB, 0x02C8, 0x02D5
        ;; 0x44..0x49
        .dw     0x02E2, 0x02EF, 0x02FC, 0x0309, 0x0316, 0x0323
        ;; 0x4A..0x4F
        .dw     0x0330, 0x033D, 0x034A, 0x0357, 0x0364, 0x0371
        ;; 0x50..0x55
        .dw     0x037E, 0x038B, 0x0398, 0x03A5, 0x03B2, 0x03BF
        ;; 0x56..0x5B
        .dw     0x03CC, 0x03D9, 0x03E6, 0x03F3, 0x0400, 0x040D
        ;; 0x5C..0x61
        .dw     0x041A, 0x0427, 0x0434, 0x0441, 0x044E, 0x045B
        ;; 0x62..0x67
        .dw     0x0468, 0x0475, 0x0482, 0x048F, 0x049C, 0x04A9
        ;; 0x68..0x6D
        .dw     0x04B6, 0x04C3, 0x04D0, 0x04DD, 0x04EA, 0x04F7
        ;; 0x6E..0x73
        .dw     0x0504, 0x0511, 0x051E, 0x052B, 0x0538, 0x0545
        ;; 0x74..0x79
        .dw     0x0552, 0x055F, 0x056C, 0x0579, 0x0586, 0x0593
        ;; 0x7A..0x7F
        ;; glyph data (serialized bmp_t records)

        ;; glyph 0x20 ' '
        .db     0x00                    ; signature (BMP_ENC_1BPP, stride=1)
        .db     0x00                    ; width
        .db     0x08                    ; height
        .dw     0x0000                  ; bitmap byte size
        ;; (no bitmap payload)

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
