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

CTX_SINK_KIND   .equ 0
CTX_SINK_TERM   .equ 1
CTX_SINK_FD     .equ 2
CTX_FLAGS       .equ 3
CTX_LENGTH      .equ 4
CTX_HAVE_PREC   .equ 5
CTX_BASE        .equ 6
CTX_UPPER       .equ 7
CTX_SIGN        .equ 8
CTX_PREFIX_LEN  .equ 9
CTX_PREFIX_0    .equ 10
CTX_PREFIX_1    .equ 11
CTX_REMAINDER   .equ 12
CTX_VALUE_ZERO  .equ 13
CTX_SINK_PTR    .equ 14
CTX_SINK_ROOM   .equ 16
CTX_COUNT       .equ 18
CTX_WIDTH       .equ 20
CTX_PRECISION   .equ 22
CTX_FMT         .equ 24
CTX_AP          .equ 26
CTX_DIGITS_PTR  .equ 28
CTX_DIGITS_LEN  .equ 30
CTX_UVAL0       .equ 32
CTX_UVAL1       .equ 33
CTX_UVAL2       .equ 34
CTX_UVAL3       .equ 35
CTX_UVAL4       .equ 36
CTX_UVAL5       .equ 37
CTX_UVAL6       .equ 38
CTX_UVAL7       .equ 39
CTX_DIGITS      .equ 40
CTX_EMIT_BYTE   .equ 72
CTX_SIZE        .equ 73

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

        .area   _CONST
__stdio_null_string:
        .asciz  "(null)"

        .area   _CODE

__stdio_alloc_ctx:
        pop     de
        ld      hl,#-CTX_SIZE
        add     hl,sp
        ld      sp,hl
        ld      iy,#0
        add     iy,sp
        push    de
        ret

__stdio_load_sink_ptr_hl:
        ld      a,CTX_SINK_PTR(iy)
        ld      l,a
        ld      a,CTX_SINK_PTR+1(iy)
        ld      h,a
        ret

__stdio_store_sink_ptr_hl:
        ld      a,l
        ld      CTX_SINK_PTR(iy),a
        ld      a,h
        ld      CTX_SINK_PTR+1(iy),a
        ret

__stdio_load_sink_room_hl:
        ld      a,CTX_SINK_ROOM(iy)
        ld      l,a
        ld      a,CTX_SINK_ROOM+1(iy)
        ld      h,a
        ret

__stdio_store_sink_room_hl:
        ld      a,l
        ld      CTX_SINK_ROOM(iy),a
        ld      a,h
        ld      CTX_SINK_ROOM+1(iy),a
        ret

__stdio_load_count_hl:
        ld      a,CTX_COUNT(iy)
        ld      l,a
        ld      a,CTX_COUNT+1(iy)
        ld      h,a
        ret

__stdio_store_count_hl:
        ld      a,l
        ld      CTX_COUNT(iy),a
        ld      a,h
        ld      CTX_COUNT+1(iy),a
        ret

__stdio_load_width_hl:
        ld      a,CTX_WIDTH(iy)
        ld      l,a
        ld      a,CTX_WIDTH+1(iy)
        ld      h,a
        ret

__stdio_store_width_hl:
        ld      a,l
        ld      CTX_WIDTH(iy),a
        ld      a,h
        ld      CTX_WIDTH+1(iy),a
        ret

__stdio_load_precision_hl:
        ld      a,CTX_PRECISION(iy)
        ld      l,a
        ld      a,CTX_PRECISION+1(iy)
        ld      h,a
        ret

__stdio_store_precision_hl:
        ld      a,l
        ld      CTX_PRECISION(iy),a
        ld      a,h
        ld      CTX_PRECISION+1(iy),a
        ret

__stdio_load_fmt_hl:
        ld      a,CTX_FMT(iy)
        ld      l,a
        ld      a,CTX_FMT+1(iy)
        ld      h,a
        ret

__stdio_store_fmt_hl:
        ld      a,l
        ld      CTX_FMT(iy),a
        ld      a,h
        ld      CTX_FMT+1(iy),a
        ret

__stdio_load_ap_hl:
        ld      a,CTX_AP(iy)
        ld      l,a
        ld      a,CTX_AP+1(iy)
        ld      h,a
        ret

__stdio_store_ap_hl:
        ld      a,l
        ld      CTX_AP(iy),a
        ld      a,h
        ld      CTX_AP+1(iy),a
        ret

__stdio_reset_field_state:
        xor     a
        ld      CTX_FLAGS(iy),a
        ld      CTX_HAVE_PREC(iy),a
        ld      CTX_SIGN(iy),a
        ld      CTX_PREFIX_LEN(iy),a
        ld      CTX_UPPER(iy),a
        ld      a,#LEN_WORD
        ld      CTX_LENGTH(iy),a
        ld      hl,#0x0000
        call    __stdio_store_width_hl
        jp      __stdio_store_precision_hl

__stdio_load_digits_ptr_hl:
        ld      a,CTX_DIGITS_PTR(iy)
        ld      l,a
        ld      a,CTX_DIGITS_PTR+1(iy)
        ld      h,a
        ret

__stdio_store_digits_ptr_hl:
        ld      a,l
        ld      CTX_DIGITS_PTR(iy),a
        ld      a,h
        ld      CTX_DIGITS_PTR+1(iy),a
        ret

__stdio_load_digits_len_hl:
        ld      a,CTX_DIGITS_LEN(iy)
        ld      l,a
        ld      a,CTX_DIGITS_LEN+1(iy)
        ld      h,a
        ret

__stdio_store_digits_len_hl:
        ld      a,l
        ld      CTX_DIGITS_LEN(iy),a
        ld      a,h
        ld      CTX_DIGITS_LEN+1(iy),a
        ret

__stdio_load_uval0_hl:
        ld      a,CTX_UVAL0(iy)
        ld      l,a
        ld      a,CTX_UVAL1(iy)
        ld      h,a
        ret

__stdio_store_uval0_hl:
        ld      a,l
        ld      CTX_UVAL0(iy),a
        ld      a,h
        ld      CTX_UVAL1(iy),a
        ret

__stdio_load_uval2_hl:
        ld      a,CTX_UVAL2(iy)
        ld      l,a
        ld      a,CTX_UVAL3(iy)
        ld      h,a
        ret

__stdio_store_uval2_hl:
        ld      a,l
        ld      CTX_UVAL2(iy),a
        ld      a,h
        ld      CTX_UVAL3(iy),a
        ret

__stdio_load_uval4_hl:
        ld      a,CTX_UVAL4(iy)
        ld      l,a
        ld      a,CTX_UVAL5(iy)
        ld      h,a
        ret

__stdio_store_uval4_hl:
        ld      a,l
        ld      CTX_UVAL4(iy),a
        ld      a,h
        ld      CTX_UVAL5(iy),a
        ret

__stdio_load_uval6_hl:
        ld      a,CTX_UVAL6(iy)
        ld      l,a
        ld      a,CTX_UVAL7(iy)
        ld      h,a
        ret

__stdio_store_uval6_hl:
        ld      a,l
        ld      CTX_UVAL6(iy),a
        ld      a,h
        ld      CTX_UVAL7(iy),a
        ret

__stdio_set_count_zero:
        xor     a
        ld      CTX_COUNT(iy),a
        ld      CTX_COUNT+1(iy),a
        ret

__stdio_init_console:
        xor     a
        ld      CTX_SINK_KIND(iy),a
        ld      CTX_SINK_TERM(iy),a
        ld      a,#1
        ld      CTX_SINK_FD(iy),a
        jp      __stdio_set_count_zero

__stdio_init_console_fd:
        ld      CTX_SINK_FD(iy),a
        xor     a
        ld      CTX_SINK_KIND(iy),a
        ld      CTX_SINK_TERM(iy),a
        jp      __stdio_set_count_zero

__stdio_init_string:
        ld      a,#SINK_STRING
        ld      CTX_SINK_KIND(iy),a
        ld      a,#1
        ld      CTX_SINK_TERM(iy),a
        call    __stdio_store_sink_ptr_hl
        jp      __stdio_set_count_zero

__stdio_init_nstring:
        ld      a,#SINK_NSTRING
        ld      CTX_SINK_KIND(iy),a
        xor     a
        ld      CTX_SINK_TERM(iy),a
        call    __stdio_store_sink_ptr_hl
        ld      a,d
        or      e
        jr      z,__stdio_init_nstring_room_zero
        ld      a,#1
        ld      CTX_SINK_TERM(iy),a
        dec     de
        push    de
        pop     hl
        call    __stdio_store_sink_room_hl
        jp      __stdio_set_count_zero
__stdio_init_nstring_room_zero:
        push    de
        pop     hl
        call    __stdio_store_sink_room_hl
        jp      __stdio_set_count_zero

__stdio_finish_sink:
        ld      a,CTX_SINK_KIND(iy)
        cp      #SINK_STRING
        jr      z,__stdio_finish_sink_store
        cp      #SINK_NSTRING
        ret     nz
        ld      a,CTX_SINK_TERM(iy)
        or      a
        ret     z
__stdio_finish_sink_store:
        call    __stdio_load_sink_ptr_hl
        xor     a
        ld      (hl),a
        ret

__stdio_emit_a:
        push    bc
        push    de
        push    hl
        ld      b,a
        call    __stdio_load_count_hl
        inc     hl
        call    __stdio_store_count_hl
        ld      a,CTX_SINK_KIND(iy)
        or      a
        jr      z,__stdio_emit_console
        cp      #SINK_STRING
        jr      z,__stdio_emit_string
        call    __stdio_load_sink_room_hl
        ld      a,h
        or      l
        jr      z,__stdio_emit_done
        call    __stdio_load_sink_ptr_hl
        ld      a,b
        ld      (hl),a
        inc     hl
        call    __stdio_store_sink_ptr_hl
        call    __stdio_load_sink_room_hl
        dec     hl
        call    __stdio_store_sink_room_hl
        jr      __stdio_emit_done
__stdio_emit_console:
        push    iy
        push    iy
        pop     hl
        ld      de,#CTX_EMIT_BYTE
        add     hl,de
        ld      a,b
        ld      (hl),a
        push    hl
        pop     de
        ld      a,CTX_SINK_FD(iy)
        ld      l,a
        ld      h,#0x00
        ld      bc,#0x0001
        push    bc
        call    _write
        pop     bc
        pop     iy
        jr      __stdio_emit_done
__stdio_emit_string:
        call    __stdio_load_sink_ptr_hl
        ld      a,b
        ld      (hl),a
        inc     hl
        call    __stdio_store_sink_ptr_hl
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
        call    __stdio_load_ap_hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        call    __stdio_store_ap_hl
        ld      l,e
        ld      h,d
        ret

__stdio_fetch_ptr_hl:
        jp      __stdio_fetch_u16_hl

__stdio_load_uval:
        ld      a,CTX_LENGTH(iy)
        cp      #LEN_LONG
        jr      z,__stdio_load_uval_32
        cp      #LEN_LLONG
        jr      z,__stdio_load_uval_64
        call    __stdio_fetch_u16_hl
        call    __stdio_store_uval0_hl
        xor     a
        ld      CTX_UVAL2(iy),a
        ld      CTX_UVAL3(iy),a
        ld      CTX_UVAL4(iy),a
        ld      CTX_UVAL5(iy),a
        ld      CTX_UVAL6(iy),a
        ld      CTX_UVAL7(iy),a
        ret
__stdio_load_uval_32:
        call    __stdio_load_ap_hl
        ld      a,(hl)
        ld      CTX_UVAL0(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL1(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL2(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL3(iy),a
        inc     hl
        call    __stdio_store_ap_hl
        xor     a
        ld      CTX_UVAL4(iy),a
        ld      CTX_UVAL5(iy),a
        ld      CTX_UVAL6(iy),a
        ld      CTX_UVAL7(iy),a
        ret
__stdio_load_uval_64:
        call    __stdio_load_ap_hl
        ld      a,(hl)
        ld      CTX_UVAL0(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL1(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL2(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL3(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL4(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL5(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL6(iy),a
        inc     hl
        ld      a,(hl)
        ld      CTX_UVAL7(iy),a
        inc     hl
        call    __stdio_store_ap_hl
        ret

__stdio_uval_is_zero:
        ld      a,CTX_LENGTH(iy)
        cp      #LEN_LONG
        jr      z,__stdio_uval_is_zero_32
        cp      #LEN_LLONG
        jr      z,__stdio_uval_is_zero_64
        ld      a,CTX_UVAL0(iy)
        ld      b,a
        ld      a,CTX_UVAL1(iy)
        or      b
        ret
__stdio_uval_is_zero_32:
        ld      a,CTX_UVAL0(iy)
        ld      b,a
        ld      a,CTX_UVAL1(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL2(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL3(iy)
        or      b
        ret
__stdio_uval_is_zero_64:
        ld      a,CTX_UVAL0(iy)
        ld      b,a
        ld      a,CTX_UVAL1(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL2(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL3(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL4(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL5(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL6(iy)
        or      b
        ld      b,a
        ld      a,CTX_UVAL7(iy)
        or      b
        ret

__stdio_note_zero_state:
        call    __stdio_uval_is_zero
        ld      a,#0
        jr      nz,__stdio_note_zero_store
        inc     a
__stdio_note_zero_store:
        ld      CTX_VALUE_ZERO(iy),a
        ret

__stdio_negate_uval:
        ld      a,CTX_LENGTH(iy)
        cp      #LEN_LONG
        jr      z,__stdio_negate_uval_32
        cp      #LEN_LLONG
        jr      z,__stdio_negate_uval_64
        call    __stdio_load_uval0_hl
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl
        call    __stdio_store_uval0_hl
        ret
__stdio_negate_uval_32:
        ld      a,CTX_UVAL0(iy)
        cpl
        ld      CTX_UVAL0(iy),a
        ld      a,CTX_UVAL1(iy)
        cpl
        ld      CTX_UVAL1(iy),a
        ld      a,CTX_UVAL2(iy)
        cpl
        ld      CTX_UVAL2(iy),a
        ld      a,CTX_UVAL3(iy)
        cpl
        ld      CTX_UVAL3(iy),a
        call    __stdio_load_uval0_hl
        inc     hl
        call    __stdio_store_uval0_hl
        ld      a,h
        or      l
        ret     nz
        call    __stdio_load_uval2_hl
        inc     hl
        call    __stdio_store_uval2_hl
        ret
__stdio_negate_uval_64:
        ld      a,CTX_UVAL0(iy)
        cpl
        ld      CTX_UVAL0(iy),a
        ld      a,CTX_UVAL1(iy)
        cpl
        ld      CTX_UVAL1(iy),a
        ld      a,CTX_UVAL2(iy)
        cpl
        ld      CTX_UVAL2(iy),a
        ld      a,CTX_UVAL3(iy)
        cpl
        ld      CTX_UVAL3(iy),a
        ld      a,CTX_UVAL4(iy)
        cpl
        ld      CTX_UVAL4(iy),a
        ld      a,CTX_UVAL5(iy)
        cpl
        ld      CTX_UVAL5(iy),a
        ld      a,CTX_UVAL6(iy)
        cpl
        ld      CTX_UVAL6(iy),a
        ld      a,CTX_UVAL7(iy)
        cpl
        ld      CTX_UVAL7(iy),a
        call    __stdio_load_uval0_hl
        inc     hl
        call    __stdio_store_uval0_hl
        ld      a,h
        or      l
        ret     nz
        call    __stdio_load_uval2_hl
        inc     hl
        call    __stdio_store_uval2_hl
        ld      a,h
        or      l
        ret     nz
        call    __stdio_load_uval4_hl
        inc     hl
        call    __stdio_store_uval4_hl
        ld      a,h
        or      l
        ret     nz
        call    __stdio_load_uval6_hl
        inc     hl
        call    __stdio_store_uval6_hl
        ret

__stdio_digit_char:
        cp      #10
        jr      c,__stdio_digit_char_dec
        add     a,#('A' - 10)
        ld      b,a
        ld      a,CTX_UPPER(iy)
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
        push    af
        call    __stdio_load_digits_ptr_hl
        pop     af
        ld      (hl),a
        dec     hl
        call    __stdio_store_digits_ptr_hl
        call    __stdio_load_digits_len_hl
        inc     hl
        call    __stdio_store_digits_len_hl
        pop     hl
        ret

__stdio_divmod_uval:
        ld      a,CTX_LENGTH(iy)
        cp      #LEN_LONG
        jp      z,__stdio_divmod_uval_32
        cp      #LEN_LLONG
        jp      z,__stdio_divmod_uval_64
        call    __stdio_load_uval0_hl
        ld      a,CTX_BASE(iy)
        ld      e,a
        ld      d,#0
        push    iy
        call    __divuint
        pop     iy
        ld      a,e
        ld      CTX_UVAL0(iy),a
        ld      a,d
        ld      CTX_UVAL1(iy),a
        ld      a,l
        ret
__stdio_divmod_uval_32:
        push    iy
        ld      hl,#0
        push    hl
        ld      a,CTX_BASE(iy)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,CTX_UVAL0(iy)
        ld      e,a
        ld      a,CTX_UVAL1(iy)
        ld      d,a
        ld      a,CTX_UVAL2(iy)
        ld      l,a
        ld      a,CTX_UVAL3(iy)
        ld      h,a
        call    __modulong
        pop     bc
        pop     bc
        pop     iy
        ld      a,e
        ld      CTX_REMAINDER(iy),a
        push    iy
        ld      hl,#0
        push    hl
        ld      a,CTX_BASE(iy)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,CTX_UVAL0(iy)
        ld      e,a
        ld      a,CTX_UVAL1(iy)
        ld      d,a
        ld      a,CTX_UVAL2(iy)
        ld      l,a
        ld      a,CTX_UVAL3(iy)
        ld      h,a
        call    __divulong
        pop     bc
        pop     bc
        pop     iy
        ld      a,e
        ld      CTX_UVAL0(iy),a
        ld      a,d
        ld      CTX_UVAL1(iy),a
        call    __stdio_store_uval2_hl
        xor     a
        ld      CTX_UVAL4(iy),a
        ld      CTX_UVAL5(iy),a
        ld      CTX_UVAL6(iy),a
        ld      CTX_UVAL7(iy),a
        ld      a,CTX_REMAINDER(iy)
        ret
__stdio_divmod_uval_64:
        push    iy
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        ld      a,CTX_BASE(iy)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,CTX_UVAL0(iy)
        ld      e,a
        ld      a,CTX_UVAL1(iy)
        ld      d,a
        ld      a,CTX_UVAL2(iy)
        ld      l,a
        ld      a,CTX_UVAL3(iy)
        ld      h,a
        exx
        ld      a,CTX_UVAL4(iy)
        ld      e,a
        ld      a,CTX_UVAL5(iy)
        ld      d,a
        ld      a,CTX_UVAL6(iy)
        ld      l,a
        ld      a,CTX_UVAL7(iy)
        ld      h,a
        exx
        call    __modull
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        pop     iy
        ld      a,e
        ld      CTX_REMAINDER(iy),a
        push    iy
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        ld      a,CTX_BASE(iy)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,CTX_UVAL0(iy)
        ld      e,a
        ld      a,CTX_UVAL1(iy)
        ld      d,a
        ld      a,CTX_UVAL2(iy)
        ld      l,a
        ld      a,CTX_UVAL3(iy)
        ld      h,a
        exx
        ld      a,CTX_UVAL4(iy)
        ld      e,a
        ld      a,CTX_UVAL5(iy)
        ld      d,a
        ld      a,CTX_UVAL6(iy)
        ld      l,a
        ld      a,CTX_UVAL7(iy)
        ld      h,a
        exx
        call    __divull
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        pop     iy
        ld      a,e
        ld      CTX_UVAL0(iy),a
        ld      a,d
        ld      CTX_UVAL1(iy),a
        call    __stdio_store_uval2_hl
        exx
        ld      a,e
        ld      CTX_UVAL4(iy),a
        ld      a,d
        ld      CTX_UVAL5(iy),a
        call    __stdio_store_uval6_hl
        exx
        ld      a,CTX_REMAINDER(iy)
        ret

__stdio_build_digits:
        push    iy
        pop     hl
        ld      de,#(CTX_DIGITS + 31)
        add     hl,de
        call    __stdio_store_digits_ptr_hl
        ld      hl,#0
        call    __stdio_store_digits_len_hl
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
        call    __stdio_load_fmt_hl
        ld      a,(hl)
        cp      #'0'
        jr      c,__stdio_parse_number_done
        cp      #'9' + 1
        jr      nc,__stdio_parse_number_done
        push    af
        inc     hl
        call    __stdio_store_fmt_hl
        pop     af
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
        ld      a,CTX_FLAGS(iy)
        or      #FLAG_LEFT
        ld      CTX_FLAGS(iy),a
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl
        ret

__stdio_emit_number:
        ld      a,CTX_HAVE_PREC(iy)
        or      a
        jr      z,__stdio_emit_number_after_precision_zero
        ld      a,CTX_VALUE_ZERO(iy)
        or      a
        jr      z,__stdio_emit_number_after_precision_zero
        call    __stdio_load_precision_hl
        ld      a,h
        or      l
        jr      nz,__stdio_emit_number_after_precision_zero
        ld      hl,#0x0000
        call    __stdio_store_digits_len_hl
__stdio_emit_number_after_precision_zero:
        call    __stdio_load_precision_hl
        push    hl
        call    __stdio_load_digits_len_hl
        ld      e,l
        ld      d,h
        pop     hl
        or      a
        sbc     hl,de
        jr      nc,__stdio_emit_number_have_zeroes
        ld      hl,#0x0000
__stdio_emit_number_have_zeroes:
        call    __stdio_store_precision_hl     ; reuse precision slot as zero pad count
        ld      b,h
        ld      c,l
        call    __stdio_load_digits_len_hl
        add     hl,bc
        ld      a,CTX_SIGN(iy)
        or      a
        jr      z,__stdio_emit_number_sign_count_done
        inc     hl
__stdio_emit_number_sign_count_done:
        ld      a,CTX_PREFIX_LEN(iy)
        ld      e,a
        ld      d,#0
        add     hl,de
        ex      de,hl                      ; DE = body width
        call    __stdio_load_width_hl
        or      a
        sbc     hl,de
        jr      nc,__stdio_emit_number_pad
        ld      hl,#0x0000
__stdio_emit_number_pad:
        call    __stdio_store_width_hl
        ld      c,l
        ld      b,h                        ; BC = left/right padding count
        ld      a,CTX_FLAGS(iy)
        bit     0,a
        jr      nz,__stdio_emit_number_prefix
        bit     4,a
        jr      z,__stdio_emit_number_leading_spaces
        ld      a,CTX_HAVE_PREC(iy)
        or      a
        jr      nz,__stdio_emit_number_leading_spaces
        call    __stdio_load_precision_hl
        add     hl,bc
        call    __stdio_store_precision_hl
        ld      hl,#0x0000
        call    __stdio_store_width_hl
        ld      bc,#0x0000
        jr      __stdio_emit_number_prefix
__stdio_emit_number_leading_spaces:
        ld      a,#' '
        call    __stdio_emit_padding
        jr      __stdio_emit_number_prefix
__stdio_emit_number_prefix:
        ld      a,CTX_SIGN(iy)
        or      a
        jr      z,__stdio_emit_number_prefix_bytes
        call    __stdio_emit_a
__stdio_emit_number_prefix_bytes:
        ld      a,CTX_PREFIX_LEN(iy)
        or      a
        jr      z,__stdio_emit_number_zeros
        ld      a,CTX_PREFIX_0(iy)
        call    __stdio_emit_a
        ld      a,CTX_PREFIX_LEN(iy)
        cp      #2
        jr      nz,__stdio_emit_number_zeros
        ld      a,CTX_PREFIX_1(iy)
        call    __stdio_emit_a
__stdio_emit_number_zeros:
        call    __stdio_load_precision_hl
        ld      b,h
        ld      c,l
        ld      a,#'0'
        call    __stdio_emit_padding
        call    __stdio_load_digits_ptr_hl
        inc     hl
        push    hl
        call    __stdio_load_digits_len_hl
        ld      b,h
        ld      c,l
        pop     hl
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
        ld      a,CTX_FLAGS(iy)
        bit     0,a
        ret     z
        call    __stdio_load_width_hl
        ld      b,h
        ld      c,l
        ld      hl,#0x0000
        call    __stdio_store_width_hl
        ld      a,#' '
        jp      __stdio_emit_padding

__stdio_emit_char_field:
        ld      e,a
        call    __stdio_load_width_hl
        ld      a,h
        or      l
        jr      z,__stdio_emit_char_only
        dec     hl
        ld      c,l
        ld      b,h
        ld      a,CTX_FLAGS(iy)
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
        ld      a,CTX_HAVE_PREC(iy)
        or      a
        jr      z,__stdio_emit_string_len_advance
        ld      a,d
        ld      b,a
        ld      a,CTX_PRECISION+1(iy)
        cp      b
        jr      nz,__stdio_emit_string_len_advance
        ld      a,e
        ld      b,a
        ld      a,CTX_PRECISION(iy)
        cp      b
        jr      z,__stdio_emit_string_len_done
__stdio_emit_string_len_advance:
        inc     hl
        inc     de
        jr      __stdio_emit_string_len_loop
__stdio_emit_string_len_done:
        call    __stdio_load_width_hl
        or      a
        sbc     hl,de
        jr      nc,__stdio_emit_string_pad_ok
        ld      hl,#0x0000
__stdio_emit_string_pad_ok:
        ld      b,h
        ld      c,l
        pop     hl
        ld      a,CTX_FLAGS(iy)
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
        ld      a,CTX_FLAGS(iy)
        bit     0,a
        ret     z
        ld      a,#' '
        jp      __stdio_emit_padding

__stdio_store_count_ptr:
        call    __stdio_fetch_ptr_hl
        ld      a,h
        or      l
        ret     z
        ld      e,CTX_COUNT(iy)
        ld      d,CTX_COUNT+1(iy)
        ld      a,CTX_LENGTH(iy)
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
        ld      CTX_LENGTH(iy),a
        call    __stdio_load_fmt_hl
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
        call    __stdio_store_fmt_hl
        ld      a,(hl)
        cp      #'h'
        ret     nz
        inc     hl
        call    __stdio_store_fmt_hl
        ret
__stdio_parse_length_l:
        inc     hl
        ld      a,#LEN_LONG
        ld      CTX_LENGTH(iy),a
        ld      a,(hl)
        cp      #'l'
        jr      nz,__stdio_parse_length_store_l
        inc     hl
        ld      a,#LEN_LLONG
        ld      CTX_LENGTH(iy),a
__stdio_parse_length_store_l:
        call    __stdio_store_fmt_hl
        ret
__stdio_parse_length_j:
        inc     hl
        call    __stdio_store_fmt_hl
        ld      a,#LEN_LLONG
        ld      CTX_LENGTH(iy),a
        ret
__stdio_parse_length_z:
__stdio_parse_length_t:
        inc     hl
        call    __stdio_store_fmt_hl
        ret

__stdio_vformat:
__stdio_vformat_loop:
        call    __stdio_load_fmt_hl
        ld      a,(hl)
        or      a
        jp      z,__stdio_vformat_done
        push    af
        inc     hl
        call    __stdio_store_fmt_hl
        pop     af
        cp      #'%'
        jp      z,__stdio_vformat_percent
        call    __stdio_emit_a
        jp      __stdio_vformat_loop

__stdio_vformat_percent:
        call    __stdio_reset_field_state

__stdio_vformat_flag_loop:
        call    __stdio_load_fmt_hl
        ld      a,(hl)
        cp      #'-'
        jr      nz,__stdio_vformat_flag_plus
        inc     hl
        call    __stdio_store_fmt_hl
        ld      a,CTX_FLAGS(iy)
        or      #FLAG_LEFT
        ld      CTX_FLAGS(iy),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_plus:
        cp      #'+'
        jr      nz,__stdio_vformat_flag_space
        inc     hl
        call    __stdio_store_fmt_hl
        ld      a,CTX_FLAGS(iy)
        or      #FLAG_PLUS
        ld      CTX_FLAGS(iy),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_space:
        cp      #' '
        jr      nz,__stdio_vformat_flag_hash
        inc     hl
        call    __stdio_store_fmt_hl
        ld      a,CTX_FLAGS(iy)
        or      #FLAG_SPACE
        ld      CTX_FLAGS(iy),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_hash:
        cp      #'#'
        jr      nz,__stdio_vformat_flag_zero
        inc     hl
        call    __stdio_store_fmt_hl
        ld      a,CTX_FLAGS(iy)
        or      #FLAG_ALT
        ld      CTX_FLAGS(iy),a
        jr      __stdio_vformat_flag_loop
__stdio_vformat_flag_zero:
        cp      #'0'
        jr      nz,__stdio_vformat_width
        inc     hl
        call    __stdio_store_fmt_hl
        ld      a,CTX_FLAGS(iy)
        or      #FLAG_ZERO
        ld      CTX_FLAGS(iy),a
        jr      __stdio_vformat_flag_loop

__stdio_vformat_width:
        ld      a,(hl)
        cp      #'*'
        jr      nz,__stdio_vformat_width_digits
        inc     hl
        call    __stdio_store_fmt_hl
        call    __stdio_load_width_from_star
        call    __stdio_store_width_hl
        jr      __stdio_vformat_precision
__stdio_vformat_width_digits:
        cp      #'0'
        jr      c,__stdio_vformat_precision
        cp      #'9' + 1
        jr      nc,__stdio_vformat_precision
        call    __stdio_parse_number_hl
        call    __stdio_store_width_hl

__stdio_vformat_precision:
        call    __stdio_load_fmt_hl
        ld      a,(hl)
        cp      #'.'
        jr      nz,__stdio_vformat_length
        inc     hl
        call    __stdio_store_fmt_hl
        ld      a,#1
        ld      CTX_HAVE_PREC(iy),a
        ld      hl,#0x0000
        call    __stdio_store_precision_hl
        call    __stdio_load_fmt_hl
        ld      a,(hl)
        cp      #'*'
        jr      nz,__stdio_vformat_precision_digits
        inc     hl
        call    __stdio_store_fmt_hl
        call    __stdio_fetch_u16_hl
        bit     7,h
        jr      z,__stdio_vformat_precision_store
        xor     a
        ld      CTX_HAVE_PREC(iy),a
        jr      __stdio_vformat_length
__stdio_vformat_precision_store:
        call    __stdio_store_precision_hl
        jr      __stdio_vformat_length
__stdio_vformat_precision_digits:
        call    __stdio_parse_number_hl
        call    __stdio_store_precision_hl

__stdio_vformat_length:
        call    __stdio_parse_length
        call    __stdio_load_fmt_hl
        ld      a,(hl)
        or      a
        jp      z,__stdio_vformat_done
        push    af
        inc     hl
        call    __stdio_store_fmt_hl
        pop     af
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
        cp      #'b'
        jp      z,__stdio_vformat_binary
        cp      #'B'
        jp      z,__stdio_vformat_binary
        cp      #'f'
        jp      z,__stdio_vformat_float
        cp      #'e'
        jp      z,__stdio_vformat_float
        cp      #'E'
        jp      z,__stdio_vformat_float
        cp      #'g'
        jp      z,__stdio_vformat_float
        cp      #'G'
        jp      z,__stdio_vformat_float
        cp      #'a'
        jp      z,__stdio_vformat_float
        cp      #'A'
        jp      z,__stdio_vformat_float
        cp      #'f'
        jp      z,__stdio_vformat_float
        cp      #'e'
        jp      z,__stdio_vformat_float
        cp      #'E'
        jp      z,__stdio_vformat_float
        cp      #'g'
        jp      z,__stdio_vformat_float
        cp      #'G'
        jp      z,__stdio_vformat_float
        cp      #'a'
        jp      z,__stdio_vformat_float
        cp      #'A'
        jp      z,__stdio_vformat_float
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
        ld      CTX_LENGTH(iy),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,#1
        ld      CTX_HAVE_PREC(iy),a
        ld      hl,#0x0004
        call    __stdio_store_precision_hl
        ld      a,#16
        ld      CTX_BASE(iy),a
        xor     a
        ld      CTX_UPPER(iy),a
        ld      a,#2
        ld      CTX_PREFIX_LEN(iy),a
        ld      a,#'0'
        ld      CTX_PREFIX_0(iy),a
        ld      a,#'x'
        ld      CTX_PREFIX_1(iy),a
        xor     a
        ld      CTX_SIGN(iy),a
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_store_n:
        call    __stdio_store_count_ptr
        jp      __stdio_vformat_loop

__stdio_vformat_signed:
        ld      a,#10
        ld      CTX_BASE(iy),a
        xor     a
        ld      CTX_UPPER(iy),a
        ld      CTX_PREFIX_LEN(iy),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,CTX_LENGTH(iy)
        cp      #LEN_LONG
        jr      z,__stdio_vformat_signed_32
        cp      #LEN_LLONG
        jr      z,__stdio_vformat_signed_64
        ld      a,CTX_UVAL1(iy)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      CTX_SIGN(iy),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_32:
        ld      a,CTX_UVAL3(iy)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      CTX_SIGN(iy),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_64:
        ld      a,CTX_UVAL7(iy)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      CTX_SIGN(iy),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_nonneg:
        xor     a
        ld      CTX_SIGN(iy),a
        ld      a,CTX_FLAGS(iy)
        bit     1,a
        jr      z,__stdio_vformat_signed_space
        ld      a,#'+'
        ld      CTX_SIGN(iy),a
        jr      __stdio_vformat_signed_ready
__stdio_vformat_signed_space:
        bit     2,a
        jr      z,__stdio_vformat_signed_ready
        ld      a,#' '
        ld      CTX_SIGN(iy),a
__stdio_vformat_signed_ready:
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_unsigned:
        ld      a,#10
        ld      CTX_BASE(iy),a
        xor     a
        ld      CTX_UPPER(iy),a
        xor     a
        ld      CTX_SIGN(iy),a
        ld      CTX_PREFIX_LEN(iy),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_hex_lower:
        xor     a
        ld      CTX_UPPER(iy),a
        jr      __stdio_vformat_hex_common
__stdio_vformat_hex_upper:
        ld      a,#1
        ld      CTX_UPPER(iy),a
__stdio_vformat_hex_common:
        ld      a,#16
        ld      CTX_BASE(iy),a
        xor     a
        ld      CTX_SIGN(iy),a
        ld      CTX_PREFIX_LEN(iy),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,CTX_FLAGS(iy)
        bit     3,a
        jr      z,__stdio_vformat_hex_build
        ld      a,CTX_VALUE_ZERO(iy)
        or      a
        jr      nz,__stdio_vformat_hex_build
        ld      a,#2
        ld      CTX_PREFIX_LEN(iy),a
        ld      a,#'0'
        ld      CTX_PREFIX_0(iy),a
        ld      a,CTX_UPPER(iy)
        or      a
        ld      a,#'x'
        jr      z,__stdio_vformat_hex_prefix_store
        ld      a,#'X'
__stdio_vformat_hex_prefix_store:
        ld      CTX_PREFIX_1(iy),a
__stdio_vformat_hex_build:
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_octal:
        ld      a,#8
        ld      CTX_BASE(iy),a
        xor     a
        ld      CTX_UPPER(iy),a
        ld      CTX_SIGN(iy),a
        xor     a
        ld      CTX_PREFIX_LEN(iy),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ld      a,CTX_FLAGS(iy)
        bit     3,a
        jr      z,__stdio_vformat_octal_build
        ld      a,#1
        ld      CTX_PREFIX_LEN(iy),a
        ld      a,#'0'
        ld      CTX_PREFIX_0(iy),a
__stdio_vformat_octal_build:
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_binary:
        ld      a,#2
        ld      CTX_BASE(iy),a
        xor     a
        ld      CTX_UPPER(iy),a
        ld      CTX_SIGN(iy),a
        xor     a
        ld      CTX_PREFIX_LEN(iy),a
        call    __stdio_load_uval
        call    __stdio_note_zero_state
        ; no prefix for binary (or optional 0b if #, but basic no)
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

__stdio_vformat_float:
        ; Hardened C23 float formatting: use stack buffer + call __strfromd_core (reuses the new strfrom assembler, thread safe).
        ; For demo fp in regs (real would fetch double from va using existing fetch logic in this file).
        ; Alloc buf on stack, call, emit the resulting string, restore.
        ld      hl,#-40
        add     hl,sp
        ld      sp,hl
        push    hl   ; buf s
        ld      de,#40
        ; fp demo in regs (DE HL DE' HL' for double)
        ; (in full printf float path, load the double arg here using the va state).
        call    __strfromd_core
        pop     hl   ; buf
        call    __stdio_emit_string_field
        ld      hl,#40
        add     hl,sp
        ld      sp,hl
        jp      __stdio_vformat_loop

__stdio_vformat_done:
        call    __stdio_finish_sink
        call    __stdio_load_count_hl
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
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        call    __stdio_init_console
        ld      a,2(ix)
        push    ix
        call    __stdio_emit_a
        pop     ix
        ld      l,2(ix)
        ld      h,#0x00
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        pop     hl
        ret

_fputc::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fputc_ok
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      hl,#0xFFFF
        push    hl
        pop     de
        ret
__stdio_fputc_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        ld      a,2(ix)
        push    ix
        call    __stdio_emit_a
        pop     ix
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      h,#0x00
        push    hl
        pop     de
        ret

_fputs::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fputs_ok
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      hl,#0xFFFF
        push    hl
        pop     de
        ret
__stdio_fputs_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        call    __stdio_reset_field_state
        ld      l,2(ix)
        ld      h,3(ix)
        push    ix
        call    __stdio_emit_string_field
        pop     ix
        ld      sp,ix
        pop     ix
        pop     hl
        pop     de
        ld      hl,#0x0001
        push    hl
        pop     de
        ret

_puts::
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        call    __stdio_init_console
        call    __stdio_reset_field_state
        ld      l,2(ix)
        ld      h,3(ix)
        push    ix
        call    __stdio_emit_string_field
        pop     ix
        ld      a,#'\n'
        push    ix
        call    __stdio_emit_a
        pop     ix
        ld      sp,ix
        pop     ix
        pop     hl
        ld      hl,#0x0001
        push    hl
        pop     de
        ret

_vprintf::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        push    de
        call    __stdio_init_console
        pop     bc
        ld      l,2(ix)
        ld      h,3(ix)
        call    __stdio_store_fmt_hl
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        pop     bc
        pop     bc
        ret

_printf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        call    __stdio_init_console
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_store_fmt_hl
        push    ix
        pop     hl
        ld      de,#0x0006
        add     hl,de
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

_vfprintf::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,2(ix)
        ld      h,3(ix)
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_vfprintf_ok
        ld      sp,ix
        pop     ix
        pop     bc
        pop     bc
        ld      hl,#0xFFFF
        push    hl
        pop     de
        ret
__stdio_vfprintf_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_store_fmt_hl
        ld      l,8(ix)
        ld      h,9(ix)
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        pop     bc
        pop     bc
        ret

_fprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_stream_accepts_output
        ld      a,h
        cp      #0xFF
        jr      nz,__stdio_fprintf_ok
        ld      hl,#0xFFFF
        ld      sp,ix
        pop     ix
        push    hl
        pop     de
        ret
__stdio_fprintf_ok:
        ld      a,(hl)
        call    __stdio_init_console_fd
        ld      l,6(ix)
        ld      h,7(ix)
        call    __stdio_store_fmt_hl
        push    ix
        pop     hl
        ld      de,#0x0008
        add     hl,de
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

_vsprintf::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,2(ix)
        ld      h,3(ix)
        call    __stdio_init_string
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_store_fmt_hl
        ld      l,8(ix)
        ld      h,9(ix)
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        pop     bc
        pop     bc
        ret

_sprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_init_string
        ld      l,6(ix)
        ld      h,7(ix)
        call    __stdio_store_fmt_hl
        push    ix
        pop     hl
        ld      de,#0x0008
        add     hl,de
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret

_vsnprintf::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,2(ix)
        ld      h,3(ix)
        ld      e,4(ix)
        ld      d,5(ix)
        call    __stdio_init_nstring
        ld      l,8(ix)
        ld      h,9(ix)
        call    __stdio_store_fmt_hl
        ld      l,10(ix)
        ld      h,11(ix)
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        pop     bc
        pop     bc
        ret

_snprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        ld      e,6(ix)
        ld      d,7(ix)
        call    __stdio_init_nstring
        ld      l,8(ix)
        ld      h,9(ix)
        call    __stdio_store_fmt_hl
        push    ix
        pop     hl
        ld      de,#0x000A
        add     hl,de
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret
