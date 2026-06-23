        ; locale_common.s
        ;
        ; Built-in "C" locale data for the xcc Z80 libc.  One locale name and
        ; one struct lconv (10 char* members at offsets 0..18, then 14 char
        ; members at 20..33, each CHAR_MAX == 0x7F to mean "unspecified").
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module locale_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __locale_name
        .globl  __locale_posix
        .globl  __locale_c

        .area   _CODE
__locale_name::
        .asciz  "C"
__locale_posix::
        .asciz  "POSIX"
__loc_dot:
        .asciz  "."
__loc_empty:
        .asciz  ""

__locale_c::
        .dw     __loc_dot               ; decimal_point
        .dw     __loc_empty             ; thousands_sep
        .dw     __loc_empty             ; grouping
        .dw     __loc_empty             ; int_curr_symbol
        .dw     __loc_empty             ; currency_symbol
        .dw     __loc_empty             ; mon_decimal_point
        .dw     __loc_empty             ; mon_thousands_sep
        .dw     __loc_empty             ; mon_grouping
        .dw     __loc_empty             ; positive_sign
        .dw     __loc_empty             ; negative_sign
        .db     0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f
        .db     0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f
