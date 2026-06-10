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

        .globl  __stdio_scan_fmt
        .globl  __stdio_scan_ap
        .globl  __stdio_scan_width
        .globl  __stdio_scan_count
        .globl  __stdio_scan_assigned
        .globl  __stdio_scan_stream
        .globl  __stdio_scan_token
        .globl  __stdio_scan_float_any
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
        .globl  __sx_acc
        .globl  __sx_neg
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

        .area   _DATA
__stdio_scan_kind:
        .db     0
__stdio_scan_eof:
        .db     0
__stdio_scan_suppress:
        .db     0
__stdio_scan_length:
        .db     0
__stdio_scan_stream::
        .dw     0
__stdio_scan_str:
        .dw     0
__stdio_scan_fmt::
        .dw     0
__stdio_scan_ap::
        .dw     0
__stdio_scan_width::
        .dw     0
__stdio_scan_count::
        .dw     0
__stdio_scan_assigned::
        .dw     0
__stdio_scan_dest:
        .dw     0
__stdio_scan_tmp_end:
        .dw     0
__stdio_scan_saved_width:
        .dw     0
__stdio_scan_uval:
        .ds     8
__stdio_scan_fval:
        .ds     8
__stdio_scan_token::
        .ds     TOK_CAP
__stdio_scan_float_any::
        .db     0

        .area   _CODE

__stdio_scan_zero_state:
        xor     a
        ld      (__stdio_scan_eof),a
        ld      (__stdio_scan_suppress),a
        ld      (__stdio_scan_length),a
        ld      (__stdio_scan_count),a
        ld      (__stdio_scan_count + 1),a
        ld      (__stdio_scan_assigned),a
        ld      (__stdio_scan_assigned + 1),a
        ret

__stdio_scan_init_stdin::
        ld      hl,(_stdin)
        jr      __stdio_scan_init_stream_shared

__stdio_scan_init_stream::
__stdio_scan_init_stream_shared:
        ld      (__stdio_scan_stream),hl
        ld      a,#SCAN_KIND_STREAM
        ld      (__stdio_scan_kind),a
        jp      __stdio_scan_zero_state

__stdio_scan_init_string::
        ld      (__stdio_scan_str),hl
        ld      a,#SCAN_KIND_STRING
        ld      (__stdio_scan_kind),a
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
        ld      a,(__stdio_scan_kind)
        cp      #SCAN_KIND_STRING
        jr      z,__stdio_scan_getc_string
        ld      hl,(__stdio_scan_stream)
        call    _fgetc
        ld      a,h
        cp      #0xff
        jr      z,__stdio_scan_getc_eof
        push    hl
        ld      hl,(__stdio_scan_count)
        inc     hl
        ld      (__stdio_scan_count),hl
        pop     hl
        pop     de
        pop     bc
        ret
__stdio_scan_getc_string:
        ld      hl,(__stdio_scan_str)
        ld      a,(hl)
        or      a
        jr      z,__stdio_scan_getc_eof
        inc     hl
        ld      (__stdio_scan_str),hl
        ld      l,a
        ld      h,#0x00
        ld      de,(__stdio_scan_count)
        inc     de
        ld      (__stdio_scan_count),de
        pop     de
        pop     bc
        ret
__stdio_scan_getc_eof:
        ld      a,#1
        ld      (__stdio_scan_eof),a
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
        ld      a,(__stdio_scan_kind)
        cp      #SCAN_KIND_STRING
        jr      z,__stdio_scan_ungetc_string
        push    hl
        ld      de,(__stdio_scan_stream)
        pop     hl
        call    _ungetc
        jr      __stdio_scan_ungetc_count
__stdio_scan_ungetc_string:
        ld      hl,(__stdio_scan_str)
        dec     hl
        ld      (__stdio_scan_str),hl
__stdio_scan_ungetc_count:
        ld      hl,(__stdio_scan_count)
        dec     hl
        ld      (__stdio_scan_count),hl
        xor     a
        ld      (__stdio_scan_eof),a
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
        ld      hl,(__stdio_scan_ap)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (__stdio_scan_ap),hl
        ld      l,e
        ld      h,d
        ret

        ;; Parse width digits at (__stdio_scan_fmt). HL = width, default 0.
__stdio_scan_parse_width:
        ld      hl,#0x0000
        ld      de,(__stdio_scan_fmt)
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
        ld      (__stdio_scan_fmt),de
        ret

        ;; Parse length modifier at (__stdio_scan_fmt) into __stdio_scan_length.
__stdio_scan_parse_length:
        xor     a
        ld      (__stdio_scan_length),a
        ld      hl,(__stdio_scan_fmt)
        ld      a,(hl)
        cp      #'h'
        jr      nz,__stdio_scan_parse_length_l
        inc     hl
        ld      a,#SCAN_LEN_SHORT
        ld      (__stdio_scan_length),a
        ld      a,(hl)
        cp      #'h'
        jr      nz,__stdio_scan_parse_length_store
        inc     hl
        ld      a,#SCAN_LEN_CHAR
        ld      (__stdio_scan_length),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_l:
        ld      a,(hl)
        cp      #'l'
        jr      nz,__stdio_scan_parse_length_j
        inc     hl
        ld      a,#SCAN_LEN_LONG
        ld      (__stdio_scan_length),a
        ld      a,(hl)
        cp      #'l'
        jr      nz,__stdio_scan_parse_length_store
        inc     hl
        ld      a,#SCAN_LEN_LLONG
        ld      (__stdio_scan_length),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_j:
        cp      #'j'
        jr      nz,__stdio_scan_parse_length_zt
        inc     hl
        ld      a,#SCAN_LEN_LLONG
        ld      (__stdio_scan_length),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_zt:
        cp      #'L'
        jr      nz,__stdio_scan_parse_length_zt_lower
        inc     hl
        ld      a,#SCAN_LEN_LDOUBLE
        ld      (__stdio_scan_length),a
        jr      __stdio_scan_parse_length_store
__stdio_scan_parse_length_zt_lower:
        cp      #'z'
        jr      z,__stdio_scan_parse_length_word
        cp      #'t'
        jr      nz,__stdio_scan_parse_length_store
__stdio_scan_parse_length_word:
        inc     hl
        xor     a
        ld      (__stdio_scan_length),a
__stdio_scan_parse_length_store:
        ld      (__stdio_scan_fmt),hl
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
        ld      bc,(__stdio_scan_width)
        call    __stdio_scan_token_width
        ld      (__stdio_scan_width),bc
        ld      de,#__stdio_scan_token
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
        cp      #'p'
        jp      z,__stdio_scan_collect_hex_ptr
        jp      __stdio_scan_collect_auto

__stdio_scan_width_is_zero:
        ld      hl,(__stdio_scan_width)
        ld      a,h
        or      l
        ret

__stdio_scan_width_dec:
        ld      hl,(__stdio_scan_width)
        dec     hl
        ld      (__stdio_scan_width),hl
        ret

__stdio_scan_collect_dec:
        ld      b,#10
        jp      __stdio_scan_collect_fixed_base

__stdio_scan_collect_oct:
        ld      b,#8
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
        ld      a,(__stdio_scan_suppress)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      (__stdio_scan_dest),hl
        ld      a,(__stdio_scan_length)
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_store_n_long
        cp      #SCAN_LEN_LLONG
        jr      z,__stdio_scan_store_n_llong
        ld      de,(__stdio_scan_count)
        ld      a,(__stdio_scan_length)
        cp      #SCAN_LEN_CHAR
        jr      z,__stdio_scan_store_n_char
        ld      hl,(__stdio_scan_dest)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret
__stdio_scan_store_n_char:
        ld      hl,(__stdio_scan_dest)
        ld      (hl),e
        ret
__stdio_scan_store_n_long:
        ld      hl,(__stdio_scan_dest)
        ld      de,(__stdio_scan_count)
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
        ld      hl,(__stdio_scan_dest)
        ld      de,(__stdio_scan_count)
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
        ld      a,(__stdio_scan_suppress)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      (__stdio_scan_dest),hl
        ld      a,(__stdio_scan_length)
        cp      #SCAN_LEN_CHAR
        jr      z,__stdio_scan_store_integer_char
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_store_integer_long
        cp      #SCAN_LEN_LLONG
        jr      z,__stdio_scan_store_integer_llong
        ld      hl,(__stdio_scan_dest)
        ld      a,(__stdio_scan_uval)
        ld      (hl),a
        inc     hl
        ld      a,(__stdio_scan_uval + 1)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_integer_char:
        ld      hl,(__stdio_scan_dest)
        ld      a,(__stdio_scan_uval)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_integer_long:
        ld      hl,(__stdio_scan_dest)
        ld      a,(__stdio_scan_uval)
        ld      (hl),a
        inc     hl
        ld      a,(__stdio_scan_uval + 1)
        ld      (hl),a
        inc     hl
        ld      a,(__stdio_scan_uval + 2)
        ld      (hl),a
        inc     hl
        ld      a,(__stdio_scan_uval + 3)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_integer_llong:
        ld      hl,(__stdio_scan_dest)
        ld      de,#__stdio_scan_uval
        ld      b,#8
__stdio_scan_store_integer_llong_loop:
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        djnz    __stdio_scan_store_integer_llong_loop
        jp      __stdio_scan_inc_assigned

__stdio_scan_store_pointer:
        ld      a,(__stdio_scan_suppress)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      a,(__stdio_scan_uval)
        ld      (hl),a
        inc     hl
        ld      a,(__stdio_scan_uval + 1)
        ld      (hl),a
        jp      __stdio_scan_inc_assigned

__stdio_scan_inc_assigned:
        ld      hl,(__stdio_scan_assigned)
        inc     hl
        ld      (__stdio_scan_assigned),hl
        ret

__stdio_scan_copy_acc:
        ld      hl,#__sx_acc
        ld      de,#__stdio_scan_uval
        ld      bc,#8
        ldir
        ret

__stdio_scan_capture_float:
        ld      a,e
        ld      (__stdio_scan_fval),a
        ld      a,d
        ld      (__stdio_scan_fval + 1),a
        ld      a,l
        ld      (__stdio_scan_fval + 2),a
        ld      a,h
        ld      (__stdio_scan_fval + 3),a
        ret

__stdio_scan_capture_double:
        ld      a,e
        ld      (__stdio_scan_fval),a
        ld      a,d
        ld      (__stdio_scan_fval + 1),a
        ld      a,l
        ld      (__stdio_scan_fval + 2),a
        ld      a,h
        ld      (__stdio_scan_fval + 3),a
        exx
        ld      a,e
        ld      (__stdio_scan_fval + 4),a
        ld      a,d
        ld      (__stdio_scan_fval + 5),a
        ld      a,l
        ld      (__stdio_scan_fval + 6),a
        ld      a,h
        ld      (__stdio_scan_fval + 7),a
        exx
        ret

__stdio_scan_store_float_value:
        ld      a,(__stdio_scan_suppress)
        or      a
        ret     nz
        call    __stdio_scan_fetch_ptr
        ld      (__stdio_scan_dest),hl
        ld      a,(__stdio_scan_length)
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_store_float_double
        cp      #SCAN_LEN_LDOUBLE
        jr      z,__stdio_scan_store_float_double
        ld      hl,#__stdio_scan_fval
        ld      de,(__stdio_scan_dest)
        ld      bc,#4
        ldir
        jp      __stdio_scan_inc_assigned
__stdio_scan_store_float_double:
        ld      hl,#__stdio_scan_fval
        ld      de,(__stdio_scan_dest)
        ld      bc,#8
        ldir
        jp      __stdio_scan_inc_assigned

__stdio_scan_call_signed:
        ld      de,#__stdio_scan_tmp_end
        call    __strtox_core
        ld      a,(__sx_neg)
        or      a
        call    nz,__sx_negate
        call    __stdio_scan_copy_acc
        ret

__stdio_scan_call_unsigned:
        ld      de,#__stdio_scan_tmp_end
        call    __strtox_core
        ld      a,(__sx_neg)
        or      a
        call    nz,__sx_negate
        call    __stdio_scan_copy_acc
        ret

__stdio_scan_match_fail:
        ld      a,(__stdio_scan_eof)
        or      a
        jr      z,__stdio_scan_match_fail_count
        ld      hl,(__stdio_scan_assigned)
        ld      a,h
        or      l
        jr      nz,__stdio_scan_match_fail_count
        ld      hl,#0xffff
        ret
__stdio_scan_match_fail_count:
        ld      hl,(__stdio_scan_assigned)
        ret

        ;; Decimal float token collector.
        ;; Input:
        ;;   BC = width
        ;; Output:
        ;;   carry set on success, clear on matching failure
        ;;   __stdio_scan_token contains a NUL-terminated token
__stdio_scan_collect_float:
        ld      (__stdio_scan_width),bc
        call    __stdio_scan_token_width
        ld      (__stdio_scan_width),bc
        ld      de,#__stdio_scan_token
        xor     a
        ld      (__stdio_scan_float_any),a

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
        ld      (__stdio_scan_float_any),a
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
        ld      (__stdio_scan_float_any),a
        jr      __stdio_scan_collect_float_frac
__stdio_scan_collect_float_frac_back:
        call    __stdio_scan_ungetc
        jr      __stdio_scan_collect_float_after_mantissa
__stdio_scan_collect_float_dot_back:
        call    __stdio_scan_ungetc

__stdio_scan_collect_float_after_mantissa:
        ld      a,(__stdio_scan_float_any)
        or      a
        jp      z,__stdio_scan_collect_fail

        ;; Optional exponent. It is only committed when at least one decimal
        ;; exponent digit follows the marker (and any optional sign).
        call    __stdio_scan_width_is_zero
        jp      z,__stdio_scan_collect_float_ok
        ld      (__stdio_scan_saved_width),bc
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
        ld      bc,(__stdio_scan_saved_width)
        ld      (__stdio_scan_width),bc

__stdio_scan_collect_float_ok:
        xor     a
        ld      (de),a
        scf
        ret

        ;; Main scanner loop. Returns assignment count or EOF (-1).
__stdio_scan_core::
__stdio_scan_loop:
        ld      hl,(__stdio_scan_fmt)
        ld      a,(hl)
        or      a
        jp      z,__stdio_scan_done
        call    __stdio_scan_isspace
        jp      nz,__stdio_scan_not_format_ws
__stdio_scan_format_ws:
        inc     hl
        ld      (__stdio_scan_fmt),hl
        ld      a,(hl)
        call    __stdio_scan_isspace
        jp      z,__stdio_scan_format_ws
        call    __stdio_scan_skip_input_ws
        jp      __stdio_scan_loop
__stdio_scan_not_format_ws:
        ld      a,(hl)

__stdio_scan_literal:
        inc     hl
        ld      (__stdio_scan_fmt),hl
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
        ld      (__stdio_scan_suppress),a
        call    __stdio_scan_parse_width
        ld      (__stdio_scan_width),hl
        call    __stdio_scan_parse_length
        ld      hl,(__stdio_scan_fmt)
        ld      a,(hl)
        or      a
        jp      z,__stdio_scan_done
        cp      #'*'
        jp      nz,__stdio_scan_conv_have_spec
        ld      a,#1
        ld      (__stdio_scan_suppress),a
        inc     hl
        ld      (__stdio_scan_fmt),hl
        call    __stdio_scan_parse_width
        ld      (__stdio_scan_width),hl
        call    __stdio_scan_parse_length
        ld      hl,(__stdio_scan_fmt)
        ld      a,(hl)
        or      a
        jp      z,__stdio_scan_done
__stdio_scan_conv_have_spec:
        inc     hl
        ld      (__stdio_scan_fmt),hl
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
        ld      bc,(__stdio_scan_width)
        ld      a,b
        or      c
        jp      nz,__stdio_scan_conv_c_havew
        ld      bc,#0x0001
__stdio_scan_conv_c_havew:
        ld      a,(__stdio_scan_suppress)
        or      a
        jp      nz,__stdio_scan_conv_c_loop
        call    __stdio_scan_fetch_ptr
        ld      (__stdio_scan_dest),hl
__stdio_scan_conv_c_loop:
        ld      a,b
        or      c
        jp      z,__stdio_scan_conv_c_done
        call    __stdio_scan_getc
        ld      a,h
        cp      #0xff
        jp      z,__stdio_scan_match_fail
        ld      a,(__stdio_scan_suppress)
        or      a
        jp      nz,__stdio_scan_conv_c_next
        ld      de,(__stdio_scan_dest)
        ld      a,l
        ld      (de),a
        inc     de
        ld      (__stdio_scan_dest),de
__stdio_scan_conv_c_next:
        dec     bc
        jp      __stdio_scan_conv_c_loop
__stdio_scan_conv_c_done:
        ld      a,(__stdio_scan_suppress)
        or      a
        jp      nz,__stdio_scan_loop
        call    __stdio_scan_inc_assigned
        jp      __stdio_scan_loop

__stdio_scan_conv_s:
        call    __stdio_scan_skip_input_ws
        ld      bc,(__stdio_scan_width)
        ld      a,b
        or      c
        jp      nz,__stdio_scan_conv_s_havew
        ld      bc,#0xffff
__stdio_scan_conv_s_havew:
        xor     a
        ld      (__stdio_scan_tmp_end),a
        ld      (__stdio_scan_tmp_end + 1),a
        ld      a,(__stdio_scan_suppress)
        or      a
        jp      nz,__stdio_scan_conv_s_loop
        call    __stdio_scan_fetch_ptr
        ld      (__stdio_scan_dest),hl
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
        ld      a,(__stdio_scan_suppress)
        or      a
        jp      nz,__stdio_scan_conv_s_no_store
        ld      de,(__stdio_scan_dest)
        ld      a,l
        ld      (de),a
        inc     de
        ld      (__stdio_scan_dest),de
__stdio_scan_conv_s_no_store:
        dec     bc
        ld      hl,(__stdio_scan_tmp_end)
        inc     hl
        ld      (__stdio_scan_tmp_end),hl
        jp      __stdio_scan_conv_s_loop
__stdio_scan_conv_s_back_done:
        call    __stdio_scan_ungetc
__stdio_scan_conv_s_done:
        ld      hl,(__stdio_scan_tmp_end)
        ld      a,h
        or      l
        jp      z,__stdio_scan_match_fail
        ld      a,(__stdio_scan_suppress)
        or      a
        jp      nz,__stdio_scan_conv_s_mark
        ld      hl,(__stdio_scan_dest)
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
        ld      bc,(__stdio_scan_width)
        pop     af
        call    __stdio_scan_collect_integer
        jp      nc,__stdio_scan_match_fail
        ld      hl,#__stdio_scan_token
        call    __stdio_scan_call_signed
        call    __stdio_scan_store_integer
        jp      __stdio_scan_loop

__stdio_scan_conv_unsigned:
        push    af
        call    __stdio_scan_skip_input_ws
        ld      bc,(__stdio_scan_width)
        pop     af
        call    __stdio_scan_collect_integer
        jp      nc,__stdio_scan_match_fail
        ld      hl,#__stdio_scan_token
        call    __stdio_scan_call_unsigned
        call    __stdio_scan_store_integer
        jp      __stdio_scan_loop

__stdio_scan_conv_pointer:
        call    __stdio_scan_skip_input_ws
        ld      bc,(__stdio_scan_width)
        ld      a,#'p'
        call    __stdio_scan_collect_integer
        jp      nc,__stdio_scan_match_fail
        ld      hl,#__stdio_scan_token
        call    __stdio_scan_call_unsigned
        call    __stdio_scan_store_pointer
        jp      __stdio_scan_loop

__stdio_scan_conv_float:
        call    __stdio_scan_skip_input_ws
        ld      bc,(__stdio_scan_width)
        call    __stdio_scan_collect_float
        jp      nc,__stdio_scan_match_fail
        ld      a,(__stdio_scan_length)
        cp      #SCAN_LEN_LONG
        jr      z,__stdio_scan_conv_float_double
        cp      #SCAN_LEN_LDOUBLE
        jr      z,__stdio_scan_conv_float_ldouble
        ld      hl,#__stdio_scan_token
        ld      de,#__stdio_scan_tmp_end
        call    _strtof
        call    __stdio_scan_capture_float
        call    __stdio_scan_store_float_value
        jp      __stdio_scan_loop
__stdio_scan_conv_float_double:
        ld      hl,#__stdio_scan_token
        ld      de,#__stdio_scan_tmp_end
        call    _strtod
        call    __stdio_scan_capture_double
        call    __stdio_scan_store_float_value
        jp      __stdio_scan_loop
__stdio_scan_conv_float_ldouble:
        ld      hl,#__stdio_scan_token
        ld      de,#__stdio_scan_tmp_end
        call    _strtold
        call    __stdio_scan_capture_double
        call    __stdio_scan_store_float_value
        jp      __stdio_scan_loop

__stdio_scan_done:
        ld      hl,(__stdio_scan_assigned)
        ret
