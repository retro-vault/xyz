        ;; scanf_core.s
        ;;
        ;; Shared scanning core for the scanf family. The public entry points
        ;; live one-per-module so archive extraction stays fine-grained, while
        ;; this internal module carries the actual parser and source adapters.
        ;;
        ;; Supported conversions in this assembly-only pass:
        ;;   %d %i %u %x %X %o %c %s %p %n %%
        ;;   %f %F %e %E %g %G
        ;;
        ;; Supported modifiers:
        ;;   assignment suppression (*), width, h, hh, l, ll, j, z, t
        ;;
        ;; Scanset (%[...]) parsing and hexadecimal-float `%a` / `%A` inputs are
        ;; intentionally left for a later pass.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module scanf_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_scan_init_stdin
        .globl  __stdio_scan_init_stream
        .globl  __stdio_scan_init_string
        .globl  __stdio_scan_core

        .globl  _stdin
        .globl  _fgetc
        .globl  _ungetc
        .globl  _strtof
        .globl  _strtod
        .globl  _strtold
        .globl  __strtox_core
        .globl  __sx_negate

SCAN_KIND_STREAM        .equ 1
SCAN_KIND_STRING        .equ 2

SCAN_LEN_WORD          .equ 0
SCAN_LEN_SHORT         .equ 1
SCAN_LEN_CHAR          .equ 2
SCAN_LEN_LONG          .equ 3
SCAN_LEN_LLONG         .equ 4
SCAN_LEN_LDOUBLE       .equ 5

TOK_CAP                .equ 66

SCAN_CTX_BYTES         .equ 107
SC_KIND                .equ -107
SC_EOF                 .equ -106
SC_SUPPRESS            .equ -105
SC_LENGTH              .equ -104
SC_STREAM_LO           .equ -103
SC_STREAM_HI           .equ -102
SC_STR_LO              .equ -101
SC_STR_HI              .equ -100
SC_FMT_LO              .equ -99
SC_FMT_HI              .equ -98
SC_AP_LO               .equ -97
SC_AP_HI               .equ -96
SC_WIDTH_LO            .equ -95
SC_WIDTH_HI            .equ -94
SC_COUNT_LO            .equ -93
SC_COUNT_HI            .equ -92
SC_ASSIGNED_LO         .equ -91
SC_ASSIGNED_HI         .equ -90
SC_DEST_LO             .equ -89
SC_DEST_HI             .equ -88
SC_TMP_END_LO          .equ -87
SC_TMP_END_HI          .equ -86
SC_SAVED_WIDTH_LO      .equ -85
SC_SAVED_WIDTH_HI      .equ -84
SC_UVAL                .equ -83
SC_FVAL                .equ -75
SC_TOKEN               .equ -67
SC_FLOAT_ANY           .equ -1

        .area   _CODE

__stdio_scan_zero_state:
        xor     a
        ld      SC_EOF(ix),a
        ld      SC_SUPPRESS(ix),a
        ld      SC_LENGTH(ix),a
        ld      SC_COUNT_LO(ix),a
        ld      SC_COUNT_HI(ix),a
        ld      SC_ASSIGNED_LO(ix),a
        ld      SC_ASSIGNED_HI(ix),a
        ret

__stdio_scan_init_stdin::
        ld      hl,(_stdin)
        jr      __stdio_scan_init_stream_shared

__stdio_scan_init_stream::
__stdio_scan_init_stream_shared:
        ld      SC_STREAM_LO(ix),l
        ld      SC_STREAM_HI(ix),h
        ld      a,#SCAN_KIND_STREAM
        ld      SC_KIND(ix),a
        jp      __stdio_scan_zero_state

__stdio_scan_init_string::
        ld      SC_STR_LO(ix),l
        ld      SC_STR_HI(ix),h
        ld      a,#SCAN_KIND_STRING
        ld      SC_KIND(ix),a
        jp      __stdio_scan_zero_state

        ;; A = input byte -> Z when the byte is one of the scanf whitespace
        ;; characters consumed by format whitespace and by most conversions.
__stdio_scan_isspace:
        cp      #0x20
        ret     z
        cp      #0x09
        jr      c,__stdio_scan_isspace_no
        cp      #0x0e
        jr      c,__stdio_scan_isspace_yes
__stdio_scan_isspace_no:
        ld      a,#0x01
        or      a
        ret
__stdio_scan_isspace_yes:
        xor     a
        ret

__stdio_scan_getc:
        push    bc
        push    de
        ld      a,SC_KIND(ix)
        cp      #SCAN_KIND_STRING
        jr      z,__stdio_scan_getc_string
        ld      l,SC_STREAM_LO(ix)
        ld      h,SC_STREAM_HI(ix)
        call    _fgetc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_getc_eof
        push    hl
        ld      l,SC_COUNT_LO(ix)
        ld      h,SC_COUNT_HI(ix)
        inc     hl
        ld      SC_COUNT_LO(ix),l
        ld      SC_COUNT_HI(ix),h
        pop     hl
        pop     de
        pop     bc
        ret
__stdio_scan_getc_string:
        ld      l,SC_STR_LO(ix)
        ld      h,SC_STR_HI(ix)
        ld      a,(hl)
        or      a
        jr      z,__stdio_scan_getc_eof
        inc     hl
        ld      SC_STR_LO(ix),l
        ld      SC_STR_HI(ix),h
        ld      l,a
        ld      h,#0x00
        ld      e,SC_COUNT_LO(ix)
        ld      d,SC_COUNT_HI(ix)
        inc     de
        ld      SC_COUNT_LO(ix),e
        ld      SC_COUNT_HI(ix),d
        pop     de
        pop     bc
        ret
__stdio_scan_getc_eof:
        ld      a,#1
        ld      SC_EOF(ix),a
        ld      hl,#0xffff
        pop     de
        pop     bc
        ret

        ;; HL = character to push back (0x00xx). EOF is ignored.
__stdio_scan_ungetc:
        push    bc
        push    de
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_ungetc_done
        ld      a,SC_KIND(ix)
        cp      #SCAN_KIND_STRING
        jr      z,__stdio_scan_ungetc_string
        push    hl
        ld      e,SC_STREAM_LO(ix)
        ld      d,SC_STREAM_HI(ix)
        pop     hl
        call    _ungetc
        jr      __stdio_scan_ungetc_count
__stdio_scan_ungetc_string:
        ld      l,SC_STR_LO(ix)
        ld      h,SC_STR_HI(ix)
        dec     hl
        ld      SC_STR_LO(ix),l
        ld      SC_STR_HI(ix),h
__stdio_scan_ungetc_count:
        ld      l,SC_COUNT_LO(ix)
        ld      h,SC_COUNT_HI(ix)
        dec     hl
        ld      SC_COUNT_LO(ix),l
        ld      SC_COUNT_HI(ix),h
        xor     a
        ld      SC_EOF(ix),a
__stdio_scan_ungetc_done:
        pop     de
        pop     bc
        ret

__stdio_scan_skip_input_ws:
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        ret     z
        ld      a,l
        call    __stdio_scan_isspace
        jr      z,__stdio_scan_skip_input_ws
        jp      __stdio_scan_ungetc

__stdio_scan_fetch_ptr:
        ld      l,SC_AP_LO(ix)
        ld      h,SC_AP_HI(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      SC_AP_LO(ix),l
        ld      SC_AP_HI(ix),h
        ld      l,e
        ld      h,d
        ret

        ;; Parse width digits at (__stdio_scan_fmt). HL = width, default 0.
__stdio_scan_parse_width:
        ld      hl,#0x0000
        ld      e,SC_FMT_LO(ix)
        ld      d,SC_FMT_HI(ix)
__stdio_scan_parse_width_loop:
        ld      a,(de)
        cp      #'0'
        jr      c,__stdio_scan_parse_width_done
        cp      #'9' + 1
        jr      nc,__stdio_scan_parse_width_done
        push    de
        add     hl,hl
        push    hl
        add     hl,hl
        add     hl,hl
        pop     bc
        add     hl,bc
        ld      a,(de)
        sub     #'0'
        ld      c,a
        ld      b,#0x00
        add     hl,bc
        pop     de
        inc     de
        jr      __stdio_scan_parse_width_loop
__stdio_scan_parse_width_done:
        ld      SC_FMT_LO(ix),e
        ld      SC_FMT_HI(ix),d
        ret

        ;; Parse length modifier at (__stdio_scan_fmt) into __stdio_scan_length.
__stdio_scan_parse_length:
        xor     a
        ld      SC_LENGTH(ix),a
        ld      l,SC_FMT_LO(ix)
        ld      h,SC_FMT_HI(ix)
        ld      a,(hl)
        cp      #'h'
        jr      nz,__stdio_scan_parse_length_l
        inc     hl
        ld      a,#SCAN_LEN_SHORT
        ld      SC_LENGTH(ix),a
        ld      a,(hl)
        cp      #'h'
        jr      nz,__stdio_scan_parse_length_store
        inc     hl
        ld      a,#SCAN_LEN_CHAR
        ld      SC_LENGTH(ix),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_l:
        ld      a,(hl)
        cp      #'l'
        jr      nz,__stdio_scan_parse_length_j
        inc     hl
        ld      a,#SCAN_LEN_LONG
        ld      SC_LENGTH(ix),a
        ld      a,(hl)
        cp      #'l'
        jr      nz,__stdio_scan_parse_length_store
        inc     hl
        ld      a,#SCAN_LEN_LLONG
        ld      SC_LENGTH(ix),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_j:
        cp      #'j'
        jr      nz,__stdio_scan_parse_length_zt
        inc     hl
        ld      a,#SCAN_LEN_LLONG
        ld      SC_LENGTH(ix),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_zt:
        cp      #'L'
        jr      nz,__stdio_scan_parse_length_zt_lower
        inc     hl
        ld      a,#SCAN_LEN_LDOUBLE
        ld      SC_LENGTH(ix),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_zt_lower:
        cp      #'z'
        jr      z,__stdio_scan_parse_length_word
        cp      #'t'
        jr      nz,__stdio_scan_parse_length_store
__stdio_scan_parse_length_word:
        inc     hl
        xor     a
        ld      SC_LENGTH(ix),a
__stdio_scan_parse_length_store:
        ld      SC_FMT_LO(ix),l
        ld      SC_FMT_HI(ix),h
        ret

__stdio_scan_digitval:
        cp      #'0'
        jr      c,__stdio_scan_digitval_bad
        cp      #'9' + 1
        jr      c,__stdio_scan_digitval_dec
        cp      #'A'
        jr      c,__stdio_scan_digitval_bad
        cp      #'Z' + 1
        jr      c,__stdio_scan_digitval_up
        cp      #'a'
        jr      c,__stdio_scan_digitval_bad
        cp      #'z' + 1
        jr      c,__stdio_scan_digitval_low
__stdio_scan_digitval_bad:
        ld      a,#0xff
        ret
__stdio_scan_digitval_dec:
        sub     #'0'
        ret
__stdio_scan_digitval_up:
        sub     #0x37
        ret
__stdio_scan_digitval_low:
        sub     #0x57
        ret

        ;; A = char, C = base. Carry set when the digit is valid.
__stdio_scan_digit_for_base:
        call    __stdio_scan_digitval
        cp      c
        ccf
        ret

        ;; BC = width. Returns BC clamped to TOK_CAP-1 and non-zero.
__stdio_scan_token_width:
        ld      a,b
        or      c
        jr      nz,__stdio_scan_token_width_have
        ld      bc,#(TOK_CAP - 1)
        ret
__stdio_scan_token_width_have:
        ld      a,b
        or      a
        jr      nz,__stdio_scan_token_width_cap
        ld      a,c
        cp      #(TOK_CAP - 1)
        jr      c,__stdio_scan_token_width_keep
        jr      z,__stdio_scan_token_width_keep
__stdio_scan_token_width_cap:
        ld      bc,#(TOK_CAP - 1)
        ret
__stdio_scan_token_width_keep:
        ret

        ;; Signed integer / pointer token collector.
        ;; Input:
        ;;   A = mode ('d','i','u','o','x','X','p')
        ;; Output:
        ;;   carry set on success, clear on matching failure
        ;;   BC = base for strto* helper
        ;;   __stdio_scan_token contains a NUL-terminated token
__stdio_scan_collect_integer:
        push    af
        ld      c,SC_WIDTH_LO(ix)
        ld      b,SC_WIDTH_HI(ix)
        call    __stdio_scan_token_width
        ld      SC_WIDTH_LO(ix),c
        ld      SC_WIDTH_HI(ix),b
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        ex      de,hl
        pop     af

        ;; Optional sign for every integer conversion except %p.
        cp      #'p'
        jp      z,__stdio_scan_collect_integer_after_sign
        push    af
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_integer_fail_pop
        ld      a,l
        cp      #'+'
        jp      z,__stdio_scan_collect_integer_sign_store
        cp      #'-'
        jp      nz,__stdio_scan_collect_integer_sign_back
__stdio_scan_collect_integer_sign_store:
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        pop     af
        jp      __stdio_scan_collect_integer_after_sign
__stdio_scan_collect_integer_sign_back:
        push    hl
        call    __stdio_scan_ungetc
        pop     hl
        pop     af
        jp      __stdio_scan_collect_integer_after_sign
__stdio_scan_collect_integer_fail_pop:
        pop     af
        or      a
        ret

__stdio_scan_collect_integer_after_sign:
        cp      #'d'
        jp      z,__stdio_scan_collect_dec
        cp      #'u'
        jp      z,__stdio_scan_collect_dec
        cp      #'o'
        jp      z,__stdio_scan_collect_oct
        cp      #'x'
        jp      z,__stdio_scan_collect_hex
        cp      #'X'
        jp      z,__stdio_scan_collect_hex
        cp      #'b'
        jp      z,__stdio_scan_collect_bin
        cp      #'B'
        jp      z,__stdio_scan_collect_bin
        cp      #'p'
        jp      z,__stdio_scan_collect_hex_ptr
        jp      __stdio_scan_collect_auto

__stdio_scan_width_is_zero:
        ld      l,SC_WIDTH_LO(ix)
        ld      h,SC_WIDTH_HI(ix)
        ld      a,h
        or      l
        ret

__stdio_scan_width_dec:
        ld      l,SC_WIDTH_LO(ix)
        ld      h,SC_WIDTH_HI(ix)
        dec     hl
        ld      SC_WIDTH_LO(ix),l
        ld      SC_WIDTH_HI(ix),h
        ret

__stdio_scan_collect_dec:
        ld      b,#10
        jp      __stdio_scan_collect_fixed_base

__stdio_scan_collect_oct:
        ld      b,#8
        jp      __stdio_scan_collect_fixed_base

__stdio_scan_collect_bin:
        ld      b,#2
        jp      __stdio_scan_collect_fixed_base

__stdio_scan_collect_hex:
__stdio_scan_collect_hex_ptr:
        ld      b,#16
        jp      __stdio_scan_collect_hex_base

__stdio_scan_collect_fixed_base:
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_fail
        ld      a,l
        call    __stdio_scan_digitval
        cp      b
        jp      nc,__stdio_scan_collect_fail_back
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
__stdio_scan_collect_fixed_loop:
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_ok_base_b
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_ok_base_b
        ld      a,l
        call    __stdio_scan_digitval
        cp      b
        jp      nc,__stdio_scan_collect_fixed_back
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        jp      __stdio_scan_collect_fixed_loop
__stdio_scan_collect_fixed_back:
        call    __stdio_scan_ungetc
        jp      __stdio_scan_collect_ok_base_b

        ;; %x / %p accept an optional 0x prefix when a real hex digit follows.
__stdio_scan_collect_hex_base:
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_fail
        ld      a,l
        call    __stdio_scan_digitval
        cp      b
        jp      nc,__stdio_scan_collect_fail_back
        ld      a,l
        cp      #'0'
        jp      nz,__stdio_scan_collect_hex_store_first
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_ok_base_b
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_ok_base_b
        ld      a,l
        cp      #'x'
        jp      z,__stdio_scan_collect_hex_prefix
        cp      #'X'
        jp      z,__stdio_scan_collect_hex_prefix
        call    __stdio_scan_digitval
        cp      b
        jp      nc,__stdio_scan_collect_hex_done_back
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        jp      __stdio_scan_collect_fixed_loop
__stdio_scan_collect_hex_prefix:
        ld      c,l
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_hex_prefix_fail
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_hex_prefix_fail
        ld      a,l
        call    __stdio_scan_digitval
        cp      b
        jp      nc,__stdio_scan_collect_hex_prefix_fail_with_char
        ld      a,c
        ld      (de),a
        inc     de
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        call    __stdio_scan_width_dec
        jp      __stdio_scan_collect_fixed_loop
__stdio_scan_collect_hex_prefix_fail_with_char:
        push    hl
        call    __stdio_scan_ungetc
        pop     hl
__stdio_scan_collect_hex_prefix_fail:
        ld      l,c
        ld      h,#0x00
        call    __stdio_scan_ungetc
        jp      __stdio_scan_collect_ok_base_b
__stdio_scan_collect_hex_store_first:
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        jp      __stdio_scan_collect_fixed_loop
__stdio_scan_collect_hex_done_back:
        call    __stdio_scan_ungetc
        jp      __stdio_scan_collect_ok_base_b

        ;; %i mirrors the base-0 rules used by strtol: a 0x prefix selects
        ;; hexadecimal, a leading zero selects octal, otherwise decimal.
__stdio_scan_collect_auto:
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_fail
        ld      a,l
        cp      #'0'
        jp      z,__stdio_scan_collect_auto_zero
        cp      #'1'
        jp      c,__stdio_scan_collect_fail_back
        cp      #'9' + 1
        jp      nc,__stdio_scan_collect_fail_back
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        ld      b,#10
        jp      __stdio_scan_collect_fixed_loop
__stdio_scan_collect_auto_zero:
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        ld      b,#8
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_ok_base_b
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_ok_base_b
        ld      a,l
        cp      #'x'
        jp      z,__stdio_scan_collect_auto_prefix
        cp      #'X'
        jp      z,__stdio_scan_collect_auto_prefix
        cp      #'0'
        jp      c,__stdio_scan_collect_auto_back
        cp      #'7' + 1
        jp      nc,__stdio_scan_collect_auto_back
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        jp      __stdio_scan_collect_fixed_loop
__stdio_scan_collect_auto_prefix:
        ld      c,l
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_auto_prefix_fail
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_auto_prefix_fail
        ld      a,l
        call    __stdio_scan_digitval
        cp      #16
        jp      nc,__stdio_scan_collect_auto_prefix_fail_with_char
        ld      a,c
        ld      (de),a
        inc     de
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        call    __stdio_scan_width_dec
        ld      b,#16
        jp      __stdio_scan_collect_fixed_loop
__stdio_scan_collect_auto_prefix_fail_with_char:
        push    hl
        call    __stdio_scan_ungetc
        pop     hl
__stdio_scan_collect_auto_prefix_fail:
        ld      l,c
        ld      h,#0x00
        call    __stdio_scan_ungetc
        ld      b,#8
        jp      __stdio_scan_collect_ok_base_b
__stdio_scan_collect_auto_back:
        call    __stdio_scan_ungetc
        ld      b,#8
        jp      __stdio_scan_collect_ok_base_b

__stdio_scan_collect_fail_back:
        call    __stdio_scan_ungetc
__stdio_scan_collect_fail:
        or      a
        ret

__stdio_scan_collect_ok_base_b:
        ld      c,b
        ld      b,#0x00
        xor     a
        ld      (de),a
        scf
        ret

        ;; Store __stdio_scan_count through the current %n destination.
__stdio_scan_store_n:
        ld      a,SC_SUPPRESS(ix)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      SC_DEST_LO(ix),l
        ld      SC_DEST_HI(ix),h
        ld      a,SC_LENGTH(ix)
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_store_n_long
        cp      #SCAN_LEN_LLONG
        jr      z,__stdio_scan_store_n_llong
        ld      e,SC_COUNT_LO(ix)
        ld      d,SC_COUNT_HI(ix)
        ld      a,SC_LENGTH(ix)
        cp      #SCAN_LEN_CHAR
        jr      z,__stdio_scan_store_n_char
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret
__stdio_scan_store_n_char:
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        ld      (hl),e
        ret
__stdio_scan_store_n_long:
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        ld      e,SC_COUNT_LO(ix)
        ld      d,SC_COUNT_HI(ix)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ret
__stdio_scan_store_n_llong:
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        ld      e,SC_COUNT_LO(ix)
        ld      d,SC_COUNT_HI(ix)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        xor     a
        ld      b,#6
__stdio_scan_store_n_llong_zero:
        ld      (hl),a
        inc     hl
        djnz    __stdio_scan_store_n_llong_zero
        ret

        ;; Store __stdio_scan_uval into the destination selected by the current
        ;; length modifier. For signed conversions the parse helper has already
        ;; produced a two's-complement value.
__stdio_scan_store_integer:
        ld      a,SC_SUPPRESS(ix)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      SC_DEST_LO(ix),l
        ld      SC_DEST_HI(ix),h
        ld      a,SC_LENGTH(ix)
        cp      #SCAN_LEN_CHAR
        jr      z,__stdio_scan_store_integer_char
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_store_integer_long
        cp      #SCAN_LEN_LLONG
        jr      z,__stdio_scan_store_integer_llong
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        ld      a,SC_UVAL(ix)
        ld      (hl),a
        inc     hl
        ld      a,SC_UVAL + 1(ix)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_integer_char:
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        ld      a,SC_UVAL(ix)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_integer_long:
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        ld      a,SC_UVAL(ix)
        ld      (hl),a
        inc     hl
        ld      a,SC_UVAL + 1(ix)
        ld      (hl),a
        inc     hl
        ld      a,SC_UVAL + 2(ix)
        ld      (hl),a
        inc     hl
        ld      a,SC_UVAL + 3(ix)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_integer_llong:
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        push    ix
        pop     de
        ld      bc,#SC_UVAL
        ex      de,hl
        add     hl,bc
        ex      de,hl
        ld      b,#8
__stdio_scan_store_integer_llong_loop:
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        djnz    __stdio_scan_store_integer_llong_loop
        jp      __stdio_scan_inc_assigned

__stdio_scan_store_pointer:
        ld      a,SC_SUPPRESS(ix)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      a,SC_UVAL(ix)
        ld      (hl),a
        inc     hl
        ld      a,SC_UVAL + 1(ix)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned

__stdio_scan_inc_assigned:
        ld      l,SC_ASSIGNED_LO(ix)
        ld      h,SC_ASSIGNED_HI(ix)
        inc     hl
        ld      SC_ASSIGNED_LO(ix),l
        ld      SC_ASSIGNED_HI(ix),h
        ret

__stdio_scan_copy_acc:
        ret

__stdio_scan_capture_float:
        ld      a,e
        ld      SC_FVAL(ix),a
        ld      a,d
        ld      SC_FVAL + 1(ix),a
        ld      a,l
        ld      SC_FVAL + 2(ix),a
        ld      a,h
        ld      SC_FVAL + 3(ix),a
        ret

__stdio_scan_capture_double:
        ld      a,e
        ld      SC_FVAL(ix),a
        ld      a,d
        ld      SC_FVAL + 1(ix),a
        ld      a,l
        ld      SC_FVAL + 2(ix),a
        ld      a,h
        ld      SC_FVAL + 3(ix),a
        exx
        ld      a,e
        ld      SC_FVAL + 4(ix),a
        ld      a,d
        ld      SC_FVAL + 5(ix),a
        ld      a,l
        ld      SC_FVAL + 6(ix),a
        ld      a,h
        ld      SC_FVAL + 7(ix),a
        exx
        ret

__stdio_scan_store_float_value:
        ld      a,SC_SUPPRESS(ix)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      SC_DEST_LO(ix),l
        ld      SC_DEST_HI(ix),h
        ld      a,SC_LENGTH(ix)
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_store_float_double
        cp      #SCAN_LEN_LDOUBLE
        jr      z,__stdio_scan_store_float_double
        push    ix
        pop     hl
        ld      de,#SC_FVAL
        add     hl,de
        ld      e,SC_DEST_LO(ix)
        ld      d,SC_DEST_HI(ix)
        ld      bc,#4
        ldir
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_float_double:
        push    ix
        pop     hl
        ld      de,#SC_FVAL
        add     hl,de
        ld      e,SC_DEST_LO(ix)
        ld      d,SC_DEST_HI(ix)
        ld      bc,#8
        ldir
        jp      __stdio_scan_inc_assigned

__stdio_scan_call_signed:
        push    hl
        push    ix
        pop     hl
        ld      de,#SC_TMP_END_LO
        add     hl,de
        ex      de,hl
        push    de
        push    ix
        pop     hl
        ld      de,#SC_UVAL
        add     hl,de
        push    hl
        pop     iy
        pop     de
        pop     hl
        call    __strtox_core
        bit     1,a
        ret     z
        push    ix
        pop     hl
        ld      de,#SC_UVAL
        add     hl,de
        call    __sx_negate
        ret

__stdio_scan_call_unsigned:
        push    hl
        push    ix
        pop     hl
        ld      de,#SC_TMP_END_LO
        add     hl,de
        ex      de,hl
        push    de
        push    ix
        pop     hl
        ld      de,#SC_UVAL
        add     hl,de
        push    hl
        pop     iy
        pop     de
        pop     hl
        call    __strtox_core
        bit     1,a
        ret     z
        push    ix
        pop     hl
        ld      de,#SC_UVAL
        add     hl,de
        call    __sx_negate
        ret

__stdio_scan_match_fail:
        ld      a,SC_EOF(ix)
        or      a
        jr      z,__stdio_scan_match_fail_count
        ld      l,SC_ASSIGNED_LO(ix)
        ld      h,SC_ASSIGNED_HI(ix)
        ld      a,h
        or      l
        jr      nz,__stdio_scan_match_fail_count
        ld      hl,#0xffff
        ret
__stdio_scan_match_fail_count:
        ld      l,SC_ASSIGNED_LO(ix)
        ld      h,SC_ASSIGNED_HI(ix)
        ret

        ;; Decimal float token collector.
        ;; Input:
        ;;   BC = width
        ;; Output:
        ;;   carry set on success, clear on matching failure
        ;;   __stdio_scan_token contains a NUL-terminated token
__stdio_scan_collect_float:
        ld      SC_WIDTH_LO(ix),c
        ld      SC_WIDTH_HI(ix),b
        call    __stdio_scan_token_width
        ld      SC_WIDTH_LO(ix),c
        ld      SC_WIDTH_HI(ix),b
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        ex      de,hl
        xor     a
        ld      SC_FLOAT_ANY(ix),a

        ;; Optional sign is part of the token, but it does not by itself make
        ;; the input item valid.
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_fail
        ld      a,l
        cp      #'+'
        jr      z,__stdio_scan_collect_float_store_sign
        cp      #'-'
        jr      z,__stdio_scan_collect_float_store_sign
        call    __stdio_scan_ungetc
        jr      __stdio_scan_collect_float_integer
__stdio_scan_collect_float_store_sign:
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec

        ;; Integer digits before the optional decimal point.
__stdio_scan_collect_float_integer:
        call    __stdio_scan_width_is_zero
        jr      z,__stdio_scan_collect_float_dot
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_collect_float_dot
        ld      a,l
        call    __stdio_scan_digitval
        cp      #10
        jr      nc,__stdio_scan_collect_float_integer_back
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        ld      a,#1
        ld      SC_FLOAT_ANY(ix),a
        jr      __stdio_scan_collect_float_integer
__stdio_scan_collect_float_integer_back:
        call    __stdio_scan_ungetc

        ;; Optional decimal point and fractional digits.
__stdio_scan_collect_float_dot:
        call    __stdio_scan_width_is_zero
        jr      z,__stdio_scan_collect_float_after_mantissa
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_collect_float_after_mantissa
        ld      a,l
        cp      #'.'
        jr      nz,__stdio_scan_collect_float_dot_back
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
__stdio_scan_collect_float_frac:
        call    __stdio_scan_width_is_zero
        jr      z,__stdio_scan_collect_float_after_mantissa
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_collect_float_after_mantissa
        ld      a,l
        call    __stdio_scan_digitval
        cp      #10
        jr      nc,__stdio_scan_collect_float_frac_back
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        ld      a,#1
        ld      SC_FLOAT_ANY(ix),a
        jr      __stdio_scan_collect_float_frac
__stdio_scan_collect_float_frac_back:
        call    __stdio_scan_ungetc
        jr      __stdio_scan_collect_float_after_mantissa
__stdio_scan_collect_float_dot_back:
        call    __stdio_scan_ungetc

__stdio_scan_collect_float_after_mantissa:
        ld      a,SC_FLOAT_ANY(ix)
        or      a
        jp      z,__stdio_scan_collect_fail

        ;; Optional exponent. It is only committed when at least one decimal
        ;; exponent digit follows the marker (and any optional sign).
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_float_ok
        ld      SC_SAVED_WIDTH_LO(ix),c
        ld      SC_SAVED_WIDTH_HI(ix),b
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_collect_float_ok
        ld      a,l
        cp      #'e'
        jr      z,__stdio_scan_collect_float_exp_try
        cp      #'E'
        jr      z,__stdio_scan_collect_float_exp_try
        call    __stdio_scan_ungetc
        jp      __stdio_scan_collect_float_ok

__stdio_scan_collect_float_exp_try:
        ld      b,l                     ; exponent marker
        call    __stdio_scan_width_dec  ; reserve room for 'e' / 'E'
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_float_exp_fail_marker

        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_collect_float_exp_fail_marker
        ld      a,l
        cp      #'+'
        jr      z,__stdio_scan_collect_float_exp_have_sign
        cp      #'-'
        jr      z,__stdio_scan_collect_float_exp_have_sign
        ld      c,#0x00
        jr      __stdio_scan_collect_float_exp_first_digit

__stdio_scan_collect_float_exp_have_sign:
        ld      c,l                     ; remember exponent sign
        call    __stdio_scan_width_dec
        call    __stdio_scan_width_is_zero
        jr      z,__stdio_scan_collect_float_exp_fail_sign
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_collect_float_exp_fail_sign

__stdio_scan_collect_float_exp_first_digit:
        ld      a,l
        call    __stdio_scan_digitval
        cp      #10
        jr      nc,__stdio_scan_collect_float_exp_fail_digit
        ld      a,b
        ld      (de),a
        inc     de
        ld      a,c
        or      a
        jr      z,__stdio_scan_collect_float_exp_store_first
        ld      (de),a
        inc     de
__stdio_scan_collect_float_exp_store_first:
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
__stdio_scan_collect_float_exp_loop:
        call    __stdio_scan_width_is_zero
        jr      z,__stdio_scan_collect_float_ok
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_collect_float_ok
        ld      a,l
        call    __stdio_scan_digitval
        cp      #10
        jr      nc,__stdio_scan_collect_float_exp_back
        ld      a,l
        ld      (de),a
        inc     de
        call    __stdio_scan_width_dec
        jr      __stdio_scan_collect_float_exp_loop

__stdio_scan_collect_float_exp_back:
        call    __stdio_scan_ungetc
        jr      __stdio_scan_collect_float_ok

__stdio_scan_collect_float_exp_fail_digit:
        call    __stdio_scan_ungetc     ; push back first non-digit
        ld      a,c
        or      a
        jr      z,__stdio_scan_collect_float_exp_fail_marker
        ld      l,c
        ld      h,#0x00
        call    __stdio_scan_ungetc
        jr      __stdio_scan_collect_float_exp_fail_marker

__stdio_scan_collect_float_exp_fail_sign:
        ld      l,c
        ld      h,#0x00
        call    __stdio_scan_ungetc
__stdio_scan_collect_float_exp_fail_marker:
        ld      l,b
        ld      h,#0x00
        call    __stdio_scan_ungetc
        ld      c,SC_SAVED_WIDTH_LO(ix)
        ld      b,SC_SAVED_WIDTH_HI(ix)
        ld      SC_WIDTH_LO(ix),c
        ld      SC_WIDTH_HI(ix),b

__stdio_scan_collect_float_ok:
        xor     a
        ld      (de),a
        scf
        ret

        ;; Main scanner loop. Returns assignment count or EOF (-1).
__stdio_scan_core::
__stdio_scan_loop:
        ld      l,SC_FMT_LO(ix)
        ld      h,SC_FMT_HI(ix)
        ld      a,(hl)
        or      a
        jp      z,__stdio_scan_done
        call    __stdio_scan_isspace
        jp      nz,__stdio_scan_not_format_ws
__stdio_scan_format_ws:
        inc     hl
        ld      SC_FMT_LO(ix),l
        ld      SC_FMT_HI(ix),h
        ld      a,(hl)
        call    __stdio_scan_isspace
        jp      z,__stdio_scan_format_ws
        call    __stdio_scan_skip_input_ws
        jp      __stdio_scan_loop
__stdio_scan_not_format_ws:
        ld      a,(hl)

__stdio_scan_literal:
        inc     hl
        ld      SC_FMT_LO(ix),l
        ld      SC_FMT_HI(ix),h
        cp      #'%'
        jp      z,__stdio_scan_conv
        push    af
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_literal_fail_pop
        pop     af
        cp      l
        jp      z,__stdio_scan_loop
        push    hl
        call    __stdio_scan_ungetc
        pop     hl
        jp      __stdio_scan_match_fail
__stdio_scan_literal_fail_pop:
        pop     af
        jp      __stdio_scan_match_fail

__stdio_scan_conv:
        xor     a
        ld      SC_SUPPRESS(ix),a
        call    __stdio_scan_parse_width
        ld      SC_WIDTH_LO(ix),l
        ld      SC_WIDTH_HI(ix),h
        call    __stdio_scan_parse_length
        ld      l,SC_FMT_LO(ix)
        ld      h,SC_FMT_HI(ix)
        ld      a,(hl)
        or      a
        jp      z,__stdio_scan_done
        cp      #'*'
        jp      nz,__stdio_scan_conv_have_spec
        ld      a,#1
        ld      SC_SUPPRESS(ix),a
        inc     hl
        ld      SC_FMT_LO(ix),l
        ld      SC_FMT_HI(ix),h
        call    __stdio_scan_parse_width
        ld      SC_WIDTH_LO(ix),l
        ld      SC_WIDTH_HI(ix),h
        call    __stdio_scan_parse_length
        ld      l,SC_FMT_LO(ix)
        ld      h,SC_FMT_HI(ix)
        ld      a,(hl)
        or      a
        jp      z,__stdio_scan_done
__stdio_scan_conv_have_spec:
        inc     hl
        ld      SC_FMT_LO(ix),l
        ld      SC_FMT_HI(ix),h
        cp      #'%'
        jp      z,__stdio_scan_conv_percent
        cp      #'n'
        jp      z,__stdio_scan_conv_n
        cp      #'c'
        jp      z,__stdio_scan_conv_c
        cp      #'s'
        jp      z,__stdio_scan_conv_s
        cp      #'d'
        jp      z,__stdio_scan_conv_signed
        cp      #'i'
        jp      z,__stdio_scan_conv_signed
        cp      #'u'
        jp      z,__stdio_scan_conv_unsigned
        cp      #'o'
        jp      z,__stdio_scan_conv_unsigned
        cp      #'x'
        jp      z,__stdio_scan_conv_unsigned
        cp      #'X'
        jp      z,__stdio_scan_conv_unsigned
        cp      #'p'
        jp      z,__stdio_scan_conv_pointer
        cp      #'f'
        jp      z,__stdio_scan_conv_float
        cp      #'F'
        jp      z,__stdio_scan_conv_float
        cp      #'e'
        jp      z,__stdio_scan_conv_float
        cp      #'E'
        jp      z,__stdio_scan_conv_float
        cp      #'g'
        jp      z,__stdio_scan_conv_float
        cp      #'G'
        jp      z,__stdio_scan_conv_float
        jp      __stdio_scan_match_fail

__stdio_scan_conv_percent:
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_match_fail
        ld      a,l
        cp      #'%'
        jp      z,__stdio_scan_loop
        push    hl
        call    __stdio_scan_ungetc
        pop     hl
        jp      __stdio_scan_match_fail

__stdio_scan_conv_n:
        call    __stdio_scan_store_n
        jp      __stdio_scan_loop

__stdio_scan_conv_c:
        ld      c,SC_WIDTH_LO(ix)
        ld      b,SC_WIDTH_HI(ix)
        ld      a,b
        or      c
        jp      nz,__stdio_scan_conv_c_havew
        ld      bc,#0x0001
__stdio_scan_conv_c_havew:
        ld      a,SC_SUPPRESS(ix)
        or      a
        jp      nz,__stdio_scan_conv_c_loop
        call    __stdio_scan_fetch_ptr
        ld      SC_DEST_LO(ix),l
        ld      SC_DEST_HI(ix),h
__stdio_scan_conv_c_loop:
        ld      a,b
        or      c
        jp      z,__stdio_scan_conv_c_done
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_match_fail
        ld      a,SC_SUPPRESS(ix)
        or      a
        jp      nz,__stdio_scan_conv_c_next
        ld      e,SC_DEST_LO(ix)
        ld      d,SC_DEST_HI(ix)
        ld      a,l
        ld      (de),a
        inc     de
        ld      SC_DEST_LO(ix),e
        ld      SC_DEST_HI(ix),d
__stdio_scan_conv_c_next:
        dec     bc
        jp      __stdio_scan_conv_c_loop
__stdio_scan_conv_c_done:
        ld      a,SC_SUPPRESS(ix)
        or      a
        jp      nz,__stdio_scan_loop
        call    __stdio_scan_inc_assigned
        jp      __stdio_scan_loop

__stdio_scan_conv_s:
        call    __stdio_scan_skip_input_ws
        ld      c,SC_WIDTH_LO(ix)
        ld      b,SC_WIDTH_HI(ix)
        ld      a,b
        or      c
        jp      nz,__stdio_scan_conv_s_havew
        ld      bc,#0xffff
__stdio_scan_conv_s_havew:
        xor     a
        ld      SC_TMP_END_LO(ix),a
        ld      SC_TMP_END_HI(ix),a
        ld      a,SC_SUPPRESS(ix)
        or      a
        jp      nz,__stdio_scan_conv_s_loop
        call    __stdio_scan_fetch_ptr
        ld      SC_DEST_LO(ix),l
        ld      SC_DEST_HI(ix),h
__stdio_scan_conv_s_loop:
        ld      a,b
        or      c
        jp      z,__stdio_scan_conv_s_done
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_conv_s_done
        ld      a,l
        call    __stdio_scan_isspace
        jp      z,__stdio_scan_conv_s_back_done
        ld      a,SC_SUPPRESS(ix)
        or      a
        jp      nz,__stdio_scan_conv_s_no_store
        ld      e,SC_DEST_LO(ix)
        ld      d,SC_DEST_HI(ix)
        ld      a,l
        ld      (de),a
        inc     de
        ld      SC_DEST_LO(ix),e
        ld      SC_DEST_HI(ix),d
__stdio_scan_conv_s_no_store:
        dec     bc
        ld      l,SC_TMP_END_LO(ix)
        ld      h,SC_TMP_END_HI(ix)
        inc     hl
        ld      SC_TMP_END_LO(ix),l
        ld      SC_TMP_END_HI(ix),h
        jp      __stdio_scan_conv_s_loop
__stdio_scan_conv_s_back_done:
        call    __stdio_scan_ungetc
__stdio_scan_conv_s_done:
        ld      l,SC_TMP_END_LO(ix)
        ld      h,SC_TMP_END_HI(ix)
        ld      a,h
        or      l
        jp      z,__stdio_scan_match_fail
        ld      a,SC_SUPPRESS(ix)
        or      a
        jp      nz,__stdio_scan_conv_s_mark
        ld      l,SC_DEST_LO(ix)
        ld      h,SC_DEST_HI(ix)
        xor     a
        ld      (hl),a
__stdio_scan_conv_s_mark:
        call    __stdio_scan_inc_assigned
        jp      __stdio_scan_loop
__stdio_scan_conv_s_empty_check:
        jp      __stdio_scan_match_fail

        ;; Signed integer family uses strtoll so every length variant can
        ;; truncate from the same canonical parsed value.
__stdio_scan_conv_signed:
        push    af
        call    __stdio_scan_skip_input_ws
        ld      c,SC_WIDTH_LO(ix)
        ld      b,SC_WIDTH_HI(ix)
        pop     af
        call    __stdio_scan_collect_integer
        jp      nc,__stdio_scan_match_fail
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        call    __stdio_scan_call_signed
        call    __stdio_scan_store_integer
        jp      __stdio_scan_loop

__stdio_scan_conv_unsigned:
        push    af
        call    __stdio_scan_skip_input_ws
        ld      c,SC_WIDTH_LO(ix)
        ld      b,SC_WIDTH_HI(ix)
        pop     af
        call    __stdio_scan_collect_integer
        jp      nc,__stdio_scan_match_fail
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        call    __stdio_scan_call_unsigned
        call    __stdio_scan_store_integer
        jp      __stdio_scan_loop

__stdio_scan_conv_pointer:
        call    __stdio_scan_skip_input_ws
        ld      c,SC_WIDTH_LO(ix)
        ld      b,SC_WIDTH_HI(ix)
        ld      a,#'p'
        call    __stdio_scan_collect_integer
        jp      nc,__stdio_scan_match_fail
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        call    __stdio_scan_call_unsigned
        call    __stdio_scan_store_pointer
        jp      __stdio_scan_loop

__stdio_scan_conv_float:
        call    __stdio_scan_skip_input_ws
        ld      c,SC_WIDTH_LO(ix)
        ld      b,SC_WIDTH_HI(ix)
        call    __stdio_scan_collect_float
        jp      nc,__stdio_scan_match_fail
        ld      a,SC_LENGTH(ix)
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_conv_float_double
        cp      #SCAN_LEN_LDOUBLE
        jr      z,__stdio_scan_conv_float_ldouble
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        push    ix
        pop     de
        ld      bc,#SC_TMP_END_LO
        ex      de,hl
        add     hl,bc
        ex      de,hl
        call    _strtof
        call    __stdio_scan_capture_float
        call    __stdio_scan_store_float_value
        jp      __stdio_scan_loop
__stdio_scan_conv_float_double:
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        push    ix
        pop     de
        ld      bc,#SC_TMP_END_LO
        ex      de,hl
        add     hl,bc
        ex      de,hl
        call    _strtod
        call    __stdio_scan_capture_double
        call    __stdio_scan_store_float_value
        jp      __stdio_scan_loop
__stdio_scan_conv_float_ldouble:
        push    ix
        pop     hl
        ld      de,#SC_TOKEN
        add     hl,de
        push    ix
        pop     de
        ld      bc,#SC_TMP_END_LO
        ex      de,hl
        add     hl,bc
        ex      de,hl
        call    _strtold
        call    __stdio_scan_capture_double
        call    __stdio_scan_store_float_value
        jp      __stdio_scan_loop

__stdio_scan_done:
        ld      l,SC_ASSIGNED_LO(ix)
        ld      h,SC_ASSIGNED_HI(ix)
        ret
