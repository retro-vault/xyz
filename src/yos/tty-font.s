        ;; tty-font.s
        ;;
        ;; system (tty) font 6x6
        ;;
        ;; note:
        ;;      first ascii is 32, last ascii is 127
        ;;      there are total 96 chars size 6x6
        ;;      each char is 6 bits wide, making
        ;;      its length 6 bits * 6 rows = 36 bits.
        ;;      each char is aligned to byte hence
        ;;      there are 4 unused bits at the end
        ;;      of every char as 6x6=36 bits is needed
        ;;      and 40 bits is used per char.
        ;;      total "free" space of font is 48 bytes.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
		;; 2021-06-16   tstih
        .module font6x6

        .globl  _tty_font

        .area   _CODE
_tty_font::
        .byte   0, 0, 0, 0, 0, 32, 130, 0
        .byte   32, 0, 73, 32, 0, 0, 0, 83
        .byte   229, 62, 80, 0, 35, 238, 14, 248
        .byte   128, 203, 66, 22, 152, 0, 98, 143
        .byte   40, 216, 0, 32, 128, 0, 0, 0
        .byte   33, 4, 16, 32, 0, 32, 65, 4
        .byte   32, 0, 34, 167, 42, 32, 0, 0
        .byte   2, 28, 32, 0, 0, 0, 0, 33
        .byte   0, 0, 0, 62, 0, 0, 0, 0
        .byte   0, 32, 0, 8, 66, 16, 128, 0
        .byte   114, 106, 178, 112, 0, 33, 130, 8
        .byte   112, 0, 112, 39, 32, 248, 0, 114
        .byte   35, 34, 112, 0, 49, 73, 62, 16
        .byte   0, 250, 15, 2, 240, 0, 114, 15
        .byte   34, 112, 0, 248, 33, 8, 32, 0
        .byte   114, 39, 34, 112, 0, 114, 39, 130
        .byte   112, 0, 0, 2, 0, 32, 0, 0
        .byte   2, 0, 33, 0, 0, 2, 16, 32
        .byte   0, 0, 15, 128, 248, 0, 0, 2
        .byte   4, 32, 0, 114, 35, 0, 32, 0
        .byte   114, 235, 160, 120, 0, 114, 40, 190
        .byte   136, 0, 242, 47, 34, 240, 0, 114
        .byte   40, 34, 112, 0, 242, 40, 162, 240
        .byte   0, 250, 15, 32, 248, 0, 250, 15
        .byte   32, 128, 0, 122, 11, 162, 120, 0
        .byte   138, 47, 162, 136, 0, 112, 130, 8
        .byte   112, 0, 8, 32, 162, 112, 0, 138
        .byte   78, 36, 136, 0, 130, 8, 32, 248
        .byte   0, 139, 106, 162, 136, 0, 139, 42
        .byte   166, 136, 0, 114, 40, 162, 112, 0
        .byte   242, 40, 188, 128, 0, 114, 42, 166
        .byte   120, 0, 242, 40, 188, 136, 0, 114
        .byte   7, 2, 240, 0, 248, 130, 8, 32
        .byte   0, 138, 40, 162, 112, 0, 138, 40
        .byte   148, 32, 0, 138, 42, 182, 136, 0
        .byte   137, 66, 20, 136, 0, 138, 39, 8
        .byte   32, 0, 248, 66, 16, 248, 0, 113
        .byte   4, 16, 112, 0, 129, 2, 4, 8
        .byte   0, 112, 65, 4, 112, 0, 33, 64
        .byte   0, 0, 0, 0, 0, 0, 248, 0
        .byte   64, 128, 0, 0, 0, 1, 200, 162
        .byte   120, 0, 131, 200, 162, 240, 0, 1
        .byte   200, 160, 120, 0, 9, 232, 162, 120
        .byte   0, 1, 201, 168, 120, 0, 24, 135
        .byte   8, 32, 0, 1, 232, 190, 11, 192
        .byte   131, 200, 162, 136, 0, 32, 6, 8
        .byte   112, 0, 32, 6, 8, 33, 0, 130
        .byte   74, 52, 136, 0, 96, 130, 8, 112
        .byte   0, 3, 202, 170, 168, 0, 3, 200
        .byte   162, 136, 0, 1, 200, 162, 112, 0
        .byte   3, 200, 162, 242, 0, 1, 232, 162
        .byte   120, 32, 2, 204, 160, 128, 0, 0
        .byte   230, 6, 240, 0, 33, 194, 8, 24
        .byte   0, 2, 40, 162, 120, 0, 2, 40
        .byte   148, 32, 0, 2, 42, 170, 80, 0
        .byte   1, 66, 20, 136, 0, 2, 40, 158
        .byte   11, 192, 3, 225, 24, 248, 0, 56
        .byte   142, 8, 56, 0, 32, 130, 8, 32
        .byte   0, 224, 131, 136, 224, 0, 41, 64
        .byte   0, 0, 0, 250, 46, 162, 248, 0
