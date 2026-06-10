        ;; printf.s
        ;;
        ;; Minimal output-oriented stdio core for the xcc Z80 libc.
        ;; All public entry points use sdcccall(0), so every fixed and variadic
        ;; argument lives on the stack in a simple linear layout.
        ;;
        ;; Supported conversions:
        ;;   %d %i %u %x %X %o %c %s %p %n %%
        ;;
        ;; Supported flags / fields:
        ;;   - + space # 0
        ;;   width, precision, *, h, hh, l, ll, j, z, t
        ;;
        ;; Floating-point formatting is intentionally left for a later pass.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module printf
        .optsdcc -mz80 sdcccall(1)

        .globl  _stdin
        .globl  _stdout
        .globl  _stderr
        .globl  _putchar
        .globl  _fputc
        .globl  _puts
        .globl  _fputs
        .globl  _printf
        .globl  _fprintf
        .globl  _sprintf
        .globl  _snprintf
        .globl  _vprintf
        .globl  _vfprintf
        .globl  _vsprintf
        .globl  _vsnprintf

        .globl  __stdio_stdin_obj
        .globl  __stdio_stdout_obj
        .globl  __stdio_stderr_obj
        .globl  _write
        .globl  __divuint
        .globl  __divulong
        .globl  __modulong
        .globl  __divull
        .globl  __modull

FLAG_LEFT       .equ 0x01
FLAG_PLUS       .equ 0x02
FLAG_SPACE      .equ 0x04
FLAG_ALT        .equ 0x08
FLAG_ZERO       .equ 0x10

SINK_CONSOLE    .equ 0x00
SINK_STRING     .equ 0x01
SINK_NSTRING    .equ 0x02

LEN_WORD        .equ 0x02
LEN_LONG        .equ 0x04
LEN_LLONG       .equ 0x08

        .area   _DATA

__stdio_stdin_obj:
        .db     0, 0, 0, 0
__stdio_stdout_obj:
        .db     1, 0, 0, 0
__stdio_stderr_obj:
        .db     2, 0, 0, 0

_stdin::
        .dw     __stdio_stdin_obj
_stdout::
        .dw     __stdio_stdout_obj
_stderr::
        .dw     __stdio_stderr_obj

__stdio_sink_kind:
        .db     0
__stdio_sink_term:
        .db     0
__stdio_sink_fd:
        .db     1
__stdio_flags:
        .db     0
__stdio_length:
        .db     LEN_WORD
__stdio_have_prec:
        .db     0
__stdio_base:
        .db     10
__stdio_upper:
        .db     0
__stdio_sign:
        .db     0
__stdio_prefix_len:
        .db     0
__stdio_prefix_0:
        .db     0
__stdio_prefix_1:
        .db     0
__stdio_remainder:
        .db     0
__stdio_value_zero:
        .db     0
__stdio_sink_ptr:
        .dw     0
__stdio_sink_room:
        .dw     0
__stdio_count:
        .dw     0
__stdio_width:
        .dw     0
__stdio_precision:
        .dw     0
__stdio_fmt:
        .dw     0
__stdio_ap:
        .dw     0
__stdio_digits_ptr:
        .dw     0
__stdio_digits_len:
        .dw     0
__stdio_uval:
        .ds     8
__stdio_digits:
        .ds     32
__stdio_emit_byte:
        .db     0

        .area   _CONST
__stdio_null_string:
        .asciz  "(null)"

        .area   _CODE

__stdio_set_count_zero:
        xor     a
        ld      (__stdio_count),a
        ld      (__stdio_count + 1),a
        ret

__stdio_init_console:
        xor     a
        ld      (__stdio_sink_kind),a
        ld      (__stdio_sink_term),a
        ld      a,#1
        ld      (__stdio_sink_fd),a
        jp      __stdio_set_count_zero

__stdio_init_console_fd:
        ld      (__stdio_sink_fd),a
        xor     a
        ld      (__stdio_sink_kind),a
        ld      (__stdio_sink_term),a
        jp      __stdio_set_count_zero

__stdio_init_string:
        ld      a,#SINK_STRING
        ld      (__stdio_sink_kind),a
        ld      a,#1
        ld      (__stdio_sink_term),a
        ld      (__stdio_sink_ptr),hl
        jp      __stdio_set_count_zero

__stdio_init_nstring:
        ld      a,#SINK_NSTRING
        ld      (__stdio_sink_kind),a
        xor     a
        ld      (__stdio_sink_term),a
        ld      (__stdio_sink_ptr),hl
        ld      a,d
        or      e
        jr      z,__stdio_init_nstring_room_zero
        ld      a,#1
        ld      (__stdio_sink_term),a
        dec     de
        push    de
        pop     hl
        ld      (__stdio_sink_room),hl
        jp      __stdio_set_count_zero
__stdio_init_nstring_room_zero:
        push    de
        pop     hl
        ld      (__stdio_sink_room),hl
        jp      __stdio_set_count_zero

__stdio_finish_sink:
        ld      a,(__stdio_sink_kind)
        cp      #SINK_STRING
        jr      z,__stdio_finish_sink_store
        cp      #SINK_NSTRING
        ret     nz
        ld      a,(__stdio_sink_term)
        or      a
        ret     z
__stdio_finish_sink_store:
        ld      hl,(__stdio_sink_ptr)
        xor     a
        ld      (hl),a
        ret

__stdio_emit_a:
        push    bc
        push    de
        push    hl
        ld      b,a
        ld      hl,(__stdio_count)
        inc     hl
        ld      (__stdio_count),hl
        ld      a,(__stdio_sink_kind)
        or      a
        jr      z,__stdio_emit_console
        cp      #SINK_STRING
        jr      z,__stdio_emit_string
        ld      hl,(__stdio_sink_room)
        ld      a,h
        or      l
        jr      z,__stdio_emit_done
        ld      hl,(__stdio_sink_ptr)
        ld      a,b
        ld      (hl),a
        inc     hl
        ld      (__stdio_sink_ptr),hl
        ld      hl,(__stdio_sink_room)
        dec     hl
        ld      (__stdio_sink_room),hl
        jr      __stdio_emit_done
__stdio_emit_console:
        ld      a,b
        ld      (__stdio_emit_byte),a
        ld      a,(__stdio_sink_fd)
        ld      l,a
        ld      h,#0x00
        ld      de,#__stdio_emit_byte
        ld      bc,#0x0001
        push    bc
        call    _write
        pop     bc
        jr      __stdio_emit_done
__stdio_emit_string:
        ld      hl,(__stdio_sink_ptr)
        ld      a,b
        ld      (hl),a
        inc     hl
        ld      (__stdio_sink_ptr),hl
__stdio_emit_done:
        pop     hl
        pop     de
        pop     bc
        ret

__stdio_emit_padding:
        push    af
        ld      a,b
        or      c
        jr      nz,__stdio_emit_padding_loop
        pop     af
        ret
__stdio_emit_padding_loop:
        pop     af
        push    af
        call    __stdio_emit_a
        dec     bc
        ld      a,b
        or      c
        jr      nz,__stdio_emit_padding_loop
        pop     af
        ret

__stdio_fetch_u16_hl:
        ld      hl,(__stdio_ap)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (__stdio_ap),hl
        ld      l,e
        ld      h,d
        ret

__stdio_fetch_ptr_hl:
        jp      __stdio_fetch_u16_hl

__stdio_load_uval:
        ld      a,(__stdio_length)
        cp      #LEN_LONG
        jr      z,__stdio_load_uval_32
        cp      #LEN_LLONG
        jr      z,__stdio_load_uval_64
        call    __stdio_fetch_u16_hl
        ld      (__stdio_uval),hl
        xor     a
        ld      (__stdio_uval + 2),a
        ld      (__stdio_uval + 3),a
        ld      (__stdio_uval + 4),a
        ld      (__stdio_uval + 5),a
        ld      (__stdio_uval + 6),a
        ld      (__stdio_uval + 7),a
        ret
__stdio_load_uval_32:
        ld      hl,(__stdio_ap)
        ld      a,(hl)
        ld      (__stdio_uval),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 1),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 2),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 3),a
        inc     hl
        ld      (__stdio_ap),hl
        xor     a
        ld      (__stdio_uval + 4),a
        ld      (__stdio_uval + 5),a
        ld      (__stdio_uval + 6),a
        ld      (__stdio_uval + 7),a
        ret
__stdio_load_uval_64:
        ld      hl,(__stdio_ap)
        ld      a,(hl)
        ld      (__stdio_uval),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 1),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 2),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 3),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 4),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 5),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 6),a
        inc     hl
        ld      a,(hl)
        ld      (__stdio_uval + 7),a
        inc     hl
        ld      (__stdio_ap),hl
        ret

__stdio_uval_is_zero:
        ld      a,(__stdio_length)
        cp      #LEN_LONG
        jr      z,__stdio_uval_is_zero_32
        cp      #LEN_LLONG
        jr      z,__stdio_uval_is_zero_64
        ld      a,(__stdio_uval)
        ld      b,a
        ld      a,(__stdio_uval + 1)
        or      b
        ret
__stdio_uval_is_zero_32:
        ld      a,(__stdio_uval)
        ld      b,a
        ld      a,(__stdio_uval + 1)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 2)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 3)
        or      b
        ret
__stdio_uval_is_zero_64:
        ld      a,(__stdio_uval)
        ld      b,a
        ld      a,(__stdio_uval + 1)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 2)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 3)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 4)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 5)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 6)
        or      b
        ld      b,a
        ld      a,(__stdio_uval + 7)
        or      b
        ret

__stdio_note_zero_state:
        call    __stdio_uval_is_zero
        ld      a,#0
        jr      nz,__stdio_note_zero_store
        inc     a
__stdio_note_zero_store:
        ld      (__stdio_value_zero),a
        ret

__stdio_negate_uval:
        ld      a,(__stdio_length)
        cp      #LEN_LONG
        jr      z,__stdio_negate_uval_32
        cp      #LEN_LLONG
        jr      z,__stdio_negate_uval_64
        ld      hl,(__stdio_uval)
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl
        ld      (__stdio_uval),hl
        ret
__stdio_negate_uval_32:
        ld      a,(__stdio_uval)
        cpl
        ld      (__stdio_uval),a
        ld      a,(__stdio_uval + 1)
        cpl
        ld      (__stdio_uval + 1),a
        ld      a,(__stdio_uval + 2)
        cpl
        ld      (__stdio_uval + 2),a
        ld      a,(__stdio_uval + 3)
        cpl
        ld      (__stdio_uval + 3),a
        ld      hl,(__stdio_uval)
        inc     hl
        ld      (__stdio_uval),hl
        ld      a,h
        or      l
        ret     nz
        ld      hl,(__stdio_uval + 2)
        inc     hl
        ld      (__stdio_uval + 2),hl
        ret
__stdio_negate_uval_64:
        ld      a,(__stdio_uval)
        cpl
        ld      (__stdio_uval),a
        ld      a,(__stdio_uval + 1)
        cpl
        ld      (__stdio_uval + 1),a
        ld      a,(__stdio_uval + 2)
        cpl
        ld      (__stdio_uval + 2),a
        ld      a,(__stdio_uval + 3)
        cpl
        ld      (__stdio_uval + 3),a
        ld      a,(__stdio_uval + 4)
        cpl
        ld      (__stdio_uval + 4),a
        ld      a,(__stdio_uval + 5)
        cpl
        ld      (__stdio_uval + 5),a
        ld      a,(__stdio_uval + 6)
        cpl
        ld      (__stdio_uval + 6),a
        ld      a,(__stdio_uval + 7)
        cpl
        ld      (__stdio_uval + 7),a
        ld      hl,(__stdio_uval)
        inc     hl
        ld      (__stdio_uval),hl
        ld      a,h
        or      l
        ret     nz
        ld      hl,(__stdio_uval + 2)
        inc     hl
        ld      (__stdio_uval + 2),hl
        ld      a,h
        or      l
        ret     nz
        ld      hl,(__stdio_uval + 4)
        inc     hl
        ld      (__stdio_uval + 4),hl
        ld      a,h
        or      l
        ret     nz
        ld      hl,(__stdio_uval + 6)
        inc     hl
        ld      (__stdio_uval + 6),hl
        ret

__stdio_digit_char:
        cp      #10
        jr      c,__stdio_digit_char_dec
        add     a,#('A' - 10)
        ld      b,a
        ld      a,(__stdio_upper)
        or      a
        ld      a,b
        ret     nz
        add     a,#('a' - 'A')
        ret
__stdio_digit_char_dec:
        add     a,#'0'
        ret

__stdio_push_digit_a:
        push    hl
        ld      hl,(__stdio_digits_ptr)
        ld      (hl),a
        dec     hl
        ld      (__stdio_digits_ptr),hl
        ld      hl,(__stdio_digits_len)
        inc     hl
        ld      (__stdio_digits_len),hl
        pop     hl
        ret

__stdio_divmod_uval:
        ld      a,(__stdio_length)
        cp      #LEN_LONG
        jr      z,__stdio_divmod_uval_32
        cp      #LEN_LLONG
        jr      z,__stdio_divmod_uval_64
        ld      hl,(__stdio_uval)
        ld      a,(__stdio_base)
        ld      e,a
        ld      d,#0
        call    __divuint
        ld      a,e
        ld      (__stdio_uval),a
        ld      a,d
        ld      (__stdio_uval + 1),a
        ld      a,l
        ret
__stdio_divmod_uval_32:
        ld      hl,#0
        push    hl
        ld      a,(__stdio_base)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,(__stdio_uval)
        ld      e,a
        ld      a,(__stdio_uval + 1)
        ld      d,a
        ld      a,(__stdio_uval + 2)
        ld      l,a
        ld      a,(__stdio_uval + 3)
        ld      h,a
        call    __modulong
        ld      a,e
        ld      (__stdio_remainder),a
        pop     bc
        pop     bc
        ld      hl,#0
        push    hl
        ld      a,(__stdio_base)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,(__stdio_uval)
        ld      e,a
        ld      a,(__stdio_uval + 1)
        ld      d,a
        ld      a,(__stdio_uval + 2)
        ld      l,a
        ld      a,(__stdio_uval + 3)
        ld      h,a
        call    __divulong
        ld      a,e
        ld      (__stdio_uval),a
        ld      a,d
        ld      (__stdio_uval + 1),a
        ld      (__stdio_uval + 2),hl
        xor     a
        ld      (__stdio_uval + 4),a
        ld      (__stdio_uval + 5),a
        ld      (__stdio_uval + 6),a
        ld      (__stdio_uval + 7),a
        pop     bc
        pop     bc
        ld      a,(__stdio_remainder)
        ret
__stdio_divmod_uval_64:
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        ld      a,(__stdio_base)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,(__stdio_uval)
        ld      e,a
        ld      a,(__stdio_uval + 1)
        ld      d,a
        ld      a,(__stdio_uval + 2)
        ld      l,a
        ld      a,(__stdio_uval + 3)
        ld      h,a
        exx
        ld      a,(__stdio_uval + 4)
        ld      e,a
        ld      a,(__stdio_uval + 5)
        ld      d,a
        ld      a,(__stdio_uval + 6)
        ld      l,a
        ld      a,(__stdio_uval + 7)
        ld      h,a
        exx
        call    __modull
        ld      a,e
        ld      (__stdio_remainder),a
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        ld      a,(__stdio_base)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,(__stdio_uval)
        ld      e,a
        ld      a,(__stdio_uval + 1)
        ld      d,a
        ld      a,(__stdio_uval + 2)
        ld      l,a
        ld      a,(__stdio_uval + 3)
        ld      h,a
        exx
        ld      a,(__stdio_uval + 4)
        ld      e,a
        ld      a,(__stdio_uval + 5)
        ld      d,a
        ld      a,(__stdio_uval + 6)
        ld      l,a
        ld      a,(__stdio_uval + 7)
        ld      h,a
        exx
        call    __divull
        ld      a,e
        ld      (__stdio_uval),a
        ld      a,d
        ld      (__stdio_uval + 1),a
        ld      (__stdio_uval + 2),hl
        exx
        ld      a,e
        ld      (__stdio_uval + 4),a
        ld      a,d
        ld      (__stdio_uval + 5),a
        ld      (__stdio_uval + 6),hl
        exx
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        ld      a,(__stdio_remainder)
        ret

__stdio_build_digits:
        ld      hl,#(__stdio_digits + 31)
        ld      (__stdio_digits_ptr),hl
        ld      hl,#0
        ld      (__stdio_digits_len),hl
        call    __stdio_uval_is_zero
        jr      nz,__stdio_build_digits_loop
        ld      a,#'0'
        jp      __stdio_push_digit_a
__stdio_build_digits_loop:
        call    __stdio_divmod_uval
        call    __stdio_digit_char
        call    __stdio_push_digit_a
        call    __stdio_uval_is_zero
        jr      nz,__stdio_build_digits_loop
        ret

__stdio_parse_number_hl:
        ld      de,#0x0000
__stdio_parse_number_loop:
        ld      hl,(__stdio_fmt)
        ld      a,(hl)
        cp      #'0'
        jr      c,__stdio_parse_number_done
        cp      #'9' + 1
        jr      nc,__stdio_parse_number_done
        inc     hl
        ld      (__stdio_fmt),hl
        sub     #'0'
        ld      c,a
        ld      l,e
        ld      h,d
        add     hl,hl
        push    hl
        add     hl,hl
        add     hl,hl
        pop     de
        add     hl,de
        ld      e,c
        ld      d,#0
        add     hl,de
        ld      d,h
        ld      e,l
        jr      __stdio_parse_number_loop
__stdio_parse_number_done:
        ld      h,d
        ld      l,e
        ret

__stdio_load_width_from_star:
        call    __stdio_fetch_u16_hl
        bit     7,h
        ret     z
        ld      a,(__stdio_flags)
        or      #FLAG_LEFT
        ld      (__stdio_flags),a
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl
        ret

__stdio_emit_number:
        ld      a,(__stdio_have_prec)
        or      a
        jr      z,__stdio_emit_number_after_precision_zero
        ld      a,(__stdio_value_zero)
        or      a
        jr      z,__stdio_emit_number_after_precision_zero
        ld      hl,(__stdio_precision)
        ld      a,h
        or      l
        jr      nz,__stdio_emit_number_after_precision_zero
        ld      hl,#0x0000
        ld      (__stdio_digits_len),hl
__stdio_emit_number_after_precision_zero:
        ld      a,(__stdio_prefix_len)
        ld      e,a
        ld      d,#0
        ld      a,(__stdio_sign)
        or      a
        jr      z,__stdio_emit_number_prefix_ready
        inc     de
__stdio_emit_number_prefix_ready:
        ld      hl,(__stdio_precision)
        ld      bc,(__stdio_digits_len)
        or      a
        sbc     hl,bc
        jr      nc,__stdio_emit_number_have_zeroes
        ld      hl,#0x0000
__stdio_emit_number_have_zeroes:
        ld      (__stdio_precision),hl     ; reuse precision slot as zero pad count
        ld      b,h
        ld      c,l
        ld      hl,(__stdio_digits_len)
        add     hl,bc
        add     hl,de
        ex      de,hl                      ; DE = body width
        ld      hl,(__stdio_width)
        or      a
        sbc     hl,de
        jr      nc,__stdio_emit_number_pad
        ld      hl,#0x0000
__stdio_emit_number_pad:
        ld      (__stdio_width),hl
        ld      c,l
        ld      b,h                        ; BC = left/right padding count
        ld      a,(__stdio_flags)
        bit     0,a
        jr      nz,__stdio_emit_number_prefix
        bit     4,a
        jr      z,__stdio_emit_number_leading_spaces
        ld      a,(__stdio_have_prec)
        or      a
        jr      nz,__stdio_emit_number_leading_spaces
        ld      hl,(__stdio_precision)
        add     hl,bc
        ld      (__stdio_precision),hl
        ld      hl,#0x0000
        ld      (__stdio_width),hl
        ld      bc,#0x0000
        jr      __stdio_emit_number_prefix
__stdio_emit_number_leading_spaces:
        ld      a,#' '
        call    __stdio_emit_padding
        jr      __stdio_emit_number_prefix
__stdio_emit_number_prefix:
        ld      a,(__stdio_sign)
        or      a
        jr      z,__stdio_emit_number_prefix_bytes
        call    __stdio_emit_a
__stdio_emit_number_prefix_bytes:
        ld      a,(__stdio_prefix_len)
        or      a
        jr      z,__stdio_emit_number_zeros
        ld      a,(__stdio_prefix_0)
        call    __stdio_emit_a
        ld      a,(__stdio_prefix_len)
        cp      #2
        jr      nz,__stdio_emit_number_zeros
        ld      a,(__stdio_prefix_1)
        call    __stdio_emit_a
__stdio_emit_number_zeros:
        ld      bc,(__stdio_precision)
        ld      a,#'0'
        call    __stdio_emit_padding
        ld      hl,(__stdio_digits_ptr)
        inc     hl
        ld      bc,(__stdio_digits_len)
        ld      a,b
        or      c
        jr      z,__stdio_emit_number_left_pad
__stdio_emit_number_digits_loop:
        ld      a,(hl)
        call    __stdio_emit_a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,__stdio_emit_number_digits_loop
__stdio_emit_number_left_pad:
        ld      a,(__stdio_flags)
        bit     0,a
        ret     z
        ld      bc,(__stdio_width)
        ld      hl,#0x0000
        ld      (__stdio_width),hl
        ld      a,#' '
        jp      __stdio_emit_padding

__stdio_emit_char_field:
        ld      e,a
        ld      hl,(__stdio_width)
        ld      a,h
        or      l
        jr      z,__stdio_emit_char_only
        dec     hl
        ld      c,l
        ld      b,h
        ld      a,(__stdio_flags)
        bit     0,a
        jr      nz,__stdio_emit_char_then_pad
        ld      a,#' '
        call    __stdio_emit_padding
        ld      a,e
        jr      __stdio_emit_char_only
__stdio_emit_char_then_pad:
        ld      a,e
        call    __stdio_emit_a
        ld      a,#' '
        jp      __stdio_emit_padding
__stdio_emit_char_only:
        ld      a,e
        jp      __stdio_emit_a

__stdio_emit_string_field:
        push    hl                        ; preserve start pointer for emission
        ld      de,#0x0000
__stdio_emit_string_len_loop:
        ld      a,(hl)
        or      a
        jr      z,__stdio_emit_string_len_done
        ld      a,(__stdio_have_prec)
        or      a
        jr      z,__stdio_emit_string_len_advance
        ld      a,d
        ld      b,a
        ld      a,(__stdio_precision + 1)
        cp      b
        jr      nz,__stdio_emit_string_len_advance
        ld      a,e
        ld      b,a
        ld      a,(__stdio_precision)
        cp      b
        jr      z,__stdio_emit_string_len_done
__stdio_emit_string_len_advance:
        inc     hl
        inc     de
        jr      __stdio_emit_string_len_loop
__stdio_emit_string_len_done:
        ld      hl,(__stdio_width)
        or      a
        sbc     hl,de
        jr      nc,__stdio_emit_string_pad_ok
        ld      hl,#0x0000
__stdio_emit_string_pad_ok:
        ld      b,h
        ld      c,l
        pop     hl
        ld      a,(__stdio_flags)
        bit     0,a
        jr      nz,__stdio_emit_string_body
        ld      a,#' '
        call    __stdio_emit_padding
__stdio_emit_string_body:
        ld      a,d
        or      e
        jr      z,__stdio_emit_string_tail
        push    bc
        ld      b,d
        ld      c,e
__stdio_emit_string_chars:
        ld      a,(hl)
        call    __stdio_emit_a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,__stdio_emit_string_chars
        pop     bc
__stdio_emit_string_tail:
        ld      a,(__stdio_flags)
        bit     0,a
        ret     z
        ld      a,#' '
        jp      __stdio_emit_padding

__stdio_store_count_ptr:
        call    __stdio_fetch_ptr_hl
        ld      a,h
        or      l
        ret     z
        ld      de,(__stdio_count)
        ld      a,(__stdio_length)
        cp      #LEN_LONG
        jr      z,__stdio_store_count_ptr_32
        cp      #LEN_LLONG
        jr      z,__stdio_store_count_ptr_64
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret
__stdio_store_count_ptr_32:
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ret
__stdio_store_count_ptr_64:
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ret

__stdio_parse_length:
        ld      a,#LEN_WORD
        ld      (__stdio_length),a
        ld      hl,(__stdio_fmt)
        ld      a,(hl)
        cp      #'h'
        jr      z,__stdio_parse_length_h
        cp      #'l'
        jr      z,__stdio_parse_length_l
        cp      #'j'
        jr      z,__stdio_parse_length_j
        cp      #'z'
        jr      z,__stdio_parse_length_z
        cp      #'t'
        jr      z,__stdio_parse_length_t
        ret
__stdio_parse_length_h:
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,(hl)
        cp      #'h'
        ret     nz
        inc     hl
        ld      (__stdio_fmt),hl
        ret
__stdio_parse_length_l:
        inc     hl
        ld      a,#LEN_LONG
        ld      (__stdio_length),a
        ld      a,(hl)
        cp      #'l'
        jr      nz,__stdio_parse_length_store_l
        inc     hl
        ld      a,#LEN_LLONG
        ld      (__stdio_length),a
__stdio_parse_length_store_l:
        ld      (__stdio_fmt),hl
        ret
__stdio_parse_length_j:
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,#LEN_LLONG
        ld      (__stdio_length),a
        ret
__stdio_parse_length_z:
__stdio_parse_length_t:
        inc     hl
        ld      (__stdio_fmt),hl
        ret

__stdio_vformat:
__stdio_vformat_loop:
        ld      hl,(__stdio_fmt)
        ld      a,(hl)
        or      a
        jp      z,__stdio_vformat_done
        inc     hl
        ld      (__stdio_fmt),hl
        cp      #'%'
        jp      z,__stdio_vformat_percent
        call    __stdio_emit_a
        jp      __stdio_vformat_loop

__stdio_vformat_percent:
        xor     a
        ld      (__stdio_flags),a
        ld      (__stdio_have_prec),a
        ld      (__stdio_sign),a
        ld      (__stdio_prefix_len),a
        ld      (__stdio_upper),a
        ld      hl,#0x0000
        ld      (__stdio_width),hl
        ld      (__stdio_precision),hl

__stdio_vformat_flag_loop:
        ld      hl,(__stdio_fmt)
        ld      a,(hl)
        cp      #'-'
        jr      nz,__stdio_vformat_flag_plus
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,(__stdio_flags)
        or      #FLAG_LEFT
        ld      (__stdio_flags),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_plus:
        cp      #'+'
        jr      nz,__stdio_vformat_flag_space
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,(__stdio_flags)
        or      #FLAG_PLUS
        ld      (__stdio_flags),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_space:
        cp      #' '
        jr      nz,__stdio_vformat_flag_hash
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,(__stdio_flags)
        or      #FLAG_SPACE
        ld      (__stdio_flags),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_hash:
        cp      #'#'
        jr      nz,__stdio_vformat_flag_zero
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,(__stdio_flags)
        or      #FLAG_ALT
        ld      (__stdio_flags),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_zero:
        cp      #'0'
        jr      nz,__stdio_vformat_width
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,(__stdio_flags)
        or      #FLAG_ZERO
        ld      (__stdio_flags),a
        jr      __stdio_vformat_flag_loop

__stdio_vformat_width:
        ld      a,(hl)
        cp      #'*'
        jr      nz,__stdio_vformat_width_digits
        inc     hl
        ld      (__stdio_fmt),hl
        call    __stdio_load_width_from_star
        ld      (__stdio_width),hl
        jr      __stdio_vformat_precision
__stdio_vformat_width_digits:
        cp      #'0'
        jr      c,__stdio_vformat_precision
        cp      #'9' + 1
        jr      nc,__stdio_vformat_precision
        call    __stdio_parse_number_hl
        ld      (__stdio_width),hl

__stdio_vformat_precision:
        ld      hl,(__stdio_fmt)
        ld      a,(hl)
        cp      #'.'
        jr      nz,__stdio_vformat_length
        inc     hl
        ld      (__stdio_fmt),hl
        ld      a,#1
        ld      (__stdio_have_prec),a
        ld      hl,#0x0000
        ld      (__stdio_precision),hl
        ld      hl,(__stdio_fmt)
        ld      a,(hl)
        cp      #'*'
        jr      nz,__stdio_vformat_precision_digits
        inc     hl
        ld      (__stdio_fmt),hl
        call    __stdio_fetch_u16_hl
        bit     7,h
        jr      z,__stdio_vformat_precision_store
        xor     a
        ld      (__stdio_have_prec),a
        jr      __stdio_vformat_length
__stdio_vformat_precision_store:
        ld      (__stdio_precision),hl
        jr      __stdio_vformat_length
__stdio_vformat_precision_digits:
        call    __stdio_parse_number_hl
        ld      (__stdio_precision),hl

__stdio_vformat_length:
        call    __stdio_parse_length
        ld      hl,(__stdio_fmt)
        ld      a,(hl)
        or      a
        jp      z,__stdio_vformat_done
        inc     hl
        ld      (__stdio_fmt),hl
        cp      #'%'
        jp      z,__stdio_vformat_emit_percent
        cp      #'c'
        jp      z,__stdio_vformat_emit_char
        cp      #'s'
        jp      z,__stdio_vformat_emit_string
        cp      #'p'
        jp      z,__stdio_vformat_emit_pointer
        cp      #'n'
        jp      z,__stdio_vformat_store_n
        cp      #'d'
        jp      z,__stdio_vformat_signed
        cp      #'i'
        jp      z,__stdio_vformat_signed
        cp      #'u'
        jp      z,__stdio_vformat_unsigned
        cp      #'x'
        jp      z,__stdio_vformat_hex_lower
        cp      #'X'
        jp      z,__stdio_vformat_hex_upper
        cp      #'o'
        jp      z,__stdio_vformat_octal
        push    af
        ld      a,#'%'
        call    __stdio_emit_a
        pop     af
        call    __stdio_emit_a
        jp      __stdio_vformat_loop

__stdio_vformat_emit_percent:
        ld      a,#'%'
        call    __stdio_emit_a
        jp      __stdio_vformat_loop

__stdio_vformat_emit_char:
        call    __stdio_fetch_u16_hl
        ld      a,l
        call    __stdio_emit_char_field
        jp      __stdio_vformat_loop

__stdio_vformat_emit_string:
        call    __stdio_fetch_ptr_hl
        ld      a,h
        or      l
        jr      nz,__stdio_vformat_emit_string_have
        ld      hl,#__stdio_null_string
__stdio_vformat_emit_string_have:
        call    __stdio_emit_string_field
        jp      __stdio_vformat_loop

__stdio_vformat_emit_pointer:
        ld      a,#LEN_WORD
        ld      (__stdio_length),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,#1
        ld      (__stdio_have_prec),a
        ld      hl,#0x0004
        ld      (__stdio_precision),hl
        ld      a,#16
        ld      (__stdio_base),a
        xor     a
        ld      (__stdio_upper),a
        ld      a,#2
        ld      (__stdio_prefix_len),a
        ld      a,#'0'
        ld      (__stdio_prefix_0),a
        ld      a,#'x'
        ld      (__stdio_prefix_1),a
        xor     a
        ld      (__stdio_sign),a
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_store_n:
        call    __stdio_store_count_ptr
        jp      __stdio_vformat_loop

__stdio_vformat_signed:
        ld      a,#10
        ld      (__stdio_base),a
        xor     a
        ld      (__stdio_upper),a
        ld      (__stdio_prefix_len),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,(__stdio_length)
        cp      #LEN_LONG
        jr      z,__stdio_vformat_signed_32
        cp      #LEN_LLONG
        jr      z,__stdio_vformat_signed_64
        ld      a,(__stdio_uval + 1)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      (__stdio_sign),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_32:
        ld      a,(__stdio_uval + 3)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      (__stdio_sign),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_64:
        ld      a,(__stdio_uval + 7)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      (__stdio_sign),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_nonneg:
        xor     a
        ld      (__stdio_sign),a
        ld      a,(__stdio_flags)
        bit     1,a
        jr      z,__stdio_vformat_signed_space
        ld      a,#'+'
        ld      (__stdio_sign),a
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_space:
        bit     2,a
        jr      z,__stdio_vformat_signed_ready
        ld      a,#' '
        ld      (__stdio_sign),a
__stdio_vformat_signed_ready:
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_unsigned:
        ld      a,#10
        ld      (__stdio_base),a
        xor     a
        ld      (__stdio_upper),a
        xor     a
        ld      (__stdio_sign),a
        ld      (__stdio_prefix_len),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_hex_lower:
        xor     a
        ld      (__stdio_upper),a
        jr      __stdio_vformat_hex_common
__stdio_vformat_hex_upper:
        ld      a,#1
        ld      (__stdio_upper),a
__stdio_vformat_hex_common:
        ld      a,#16
        ld      (__stdio_base),a
        xor     a
        ld      (__stdio_sign),a
        ld      (__stdio_prefix_len),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,(__stdio_flags)
        bit     3,a
        jr      z,__stdio_vformat_hex_build
        ld      a,(__stdio_value_zero)
        or      a
        jr      nz,__stdio_vformat_hex_build
        ld      a,#2
        ld      (__stdio_prefix_len),a
        ld      a,#'0'
        ld      (__stdio_prefix_0),a
        ld      a,(__stdio_upper)
        or      a
        ld      a,#'x'
        jr      z,__stdio_vformat_hex_prefix_store
        ld      a,#'X'
__stdio_vformat_hex_prefix_store:
        ld      (__stdio_prefix_1),a
__stdio_vformat_hex_build:
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_octal:
        ld      a,#8
        ld      (__stdio_base),a
        xor     a
        ld      (__stdio_upper),a
        ld      (__stdio_sign),a
        xor     a
        ld      (__stdio_prefix_len),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,(__stdio_flags)
        bit     3,a
        jr      z,__stdio_vformat_octal_build
        ld      a,#1
        ld      (__stdio_prefix_len),a
        ld      a,#'0'
        ld      (__stdio_prefix_0),a
__stdio_vformat_octal_build:
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_done:
        call    __stdio_finish_sink
        ld      hl,(__stdio_count)
        ret

__stdio_stream_accepts_output:
        ld      a,h
        or      l
        jr      z,__stdio_stream_accepts_output_fail
        ld      a,(hl)
        cp      #0x00
        jr      z,__stdio_stream_accepts_output_fail
        cp      #0xff
        jr      z,__stdio_stream_accepts_output_fail
        ret
__stdio_stream_accepts_output_fail:
        ld      hl,#0xFFFF
        or      a
        ret

_putchar::
        call    __stdio_init_console
        ld      a,l
        call    __stdio_emit_a
        ld      h,#0x00
        push    hl
        pop     de
        ret

_fputc::
        push    hl
        ex      de,hl
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fputc_ok
        pop     hl
        ld      l,#0xFF
        ld      h,#0xFF
        ret
__stdio_fputc_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        pop     hl
        ld      a,l
        call    __stdio_emit_a
        ld      h,#0x00
        push    hl
        pop     de
        ret

_fputs::
        push    hl
        ex      de,hl
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fputs_ok
        pop     hl
        ld      hl,#0xFFFF
        ret
__stdio_fputs_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        pop     hl
        call    __stdio_emit_string_field
        ld      hl,#0x0001
        push    hl
        pop     de
        ret

_puts::
        call    __stdio_init_console
        call    __stdio_emit_string_field
        ld      a,#'\n'
        call    __stdio_emit_a
        ld      hl,#0x0001
        push    hl
        pop     de
        ret

_vprintf::
        push    de
        push    hl
        call    __stdio_init_console
        pop     hl
        ld      (__stdio_fmt),hl
        pop     hl
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        push    hl
        pop     de
        ret

_printf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_init_console
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__stdio_fmt),hl
        push    ix
        pop     hl
        ld      de,#0x0006
        add     hl,de
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        push    hl
        pop     de
        pop     ix
        ret

_vfprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_vfprintf_ok
        pop     de
        ld      hl,#0xFFFF
        pop     ix
        ret
__stdio_vfprintf_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        pop     hl
        ld      (__stdio_fmt),hl
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        push    hl
        pop     de
        pop     ix
        ret

_fprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fprintf_ok
        ld      hl,#0xFFFF
        pop     ix
        ret
__stdio_fprintf_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        ld      l,6(ix)
        ld      h,7(ix)
        ld      (__stdio_fmt),hl
        push    ix
        pop     hl
        ld      de,#0x0008
        add     hl,de
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        pop     ix
        ret

_vsprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    de
        call    __stdio_init_string
        pop     hl
        ld      (__stdio_fmt),hl
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        push    hl
        pop     de
        pop     ix
        ret

_sprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_init_string
        ld      l,6(ix)
        ld      h,7(ix)
        ld      (__stdio_fmt),hl
        push    ix
        pop     hl
        ld      de,#0x0008
        add     hl,de
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        push    hl
        pop     de
        pop     ix
        ret

_vsnprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_init_nstring
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__stdio_fmt),hl
        ld      l,6(ix)
        ld      h,7(ix)
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        push    hl
        pop     de
        pop     ix
        ret

_snprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,4(ix)
        ld      h,5(ix)
        ld      e,6(ix)
        ld      d,7(ix)
        call    __stdio_init_nstring
        ld      l,8(ix)
        ld      h,9(ix)
        ld      (__stdio_fmt),hl
        push    ix
        pop     hl
        ld      de,#0x000A
        add     hl,de
        ld      (__stdio_ap),hl
        call    __stdio_vformat
        pop     ix
        ret
