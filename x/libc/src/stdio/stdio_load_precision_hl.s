        ;; stdio_load_precision_hl.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module stdio_load_precision_hl
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_vformat
        .globl  __divuint
        .if     __XCC_LIBC_LONGLONG
        .globl  __divull
        .globl  __modull
        .endif
        .if     __XCC_LIBC_LONG
        .globl  __divulong
        .globl  __modulong
        .endif
        .if     __XCC_LIBC_STDIO_FLOAT
        .globl  _stdio_format_double
        .endif
        .globl  __stdio_emit_a
        .globl  __stdio_emit_padding
        .globl  __stdio_emit_string_field
        .globl  __stdio_load_count_hl
        .globl  __stdio_load_sink_ptr_hl
        .globl  __stdio_load_width_hl
        .globl  __stdio_reset_field_state
        .globl  __stdio_store_precision_hl
        .globl  __stdio_store_width_hl

CTX_AP          .equ 26
CTX_BASE        .equ 6
CTX_COUNT       .equ 18
CTX_DIGITS      .equ 40
CTX_DIGITS_LEN  .equ 30
CTX_DIGITS_PTR  .equ 28
CTX_FLAGS       .equ 3
CTX_FMT         .equ 24
CTX_HAVE_PREC   .equ 5
CTX_LENGTH      .equ 4
CTX_PRECISION   .equ 22
CTX_PREFIX_0    .equ 10
CTX_PREFIX_1    .equ 11
CTX_PREFIX_LEN  .equ 9
CTX_REMAINDER   .equ 12
CTX_SIGN        .equ 8
CTX_SINK_KIND   .equ 0
CTX_SINK_TERM   .equ 1
CTX_UPPER       .equ 7
CTX_UVAL0       .equ 32
CTX_UVAL1       .equ 33
CTX_UVAL2       .equ 34
CTX_UVAL3       .equ 35
CTX_UVAL4       .equ 36
CTX_UVAL5       .equ 37
CTX_UVAL6       .equ 38
CTX_UVAL7       .equ 39
CTX_VALUE_ZERO  .equ 13
FLAG_ALT        .equ 0x08
FLAG_LEFT       .equ 0x01
FLAG_PLUS       .equ 0x02
FLAG_SPACE      .equ 0x04
FLAG_ZERO       .equ 0x10
LEN_LLONG       .equ 0x08
LEN_LONG        .equ 0x04
LEN_WORD        .equ 0x02
SINK_NSTRING    .equ 0x02
SINK_STRING     .equ 0x01

        .area   _CONST
__stdio_null_string:
        .asciz  "(null)"

        .area   _CODE
__stdio_load_precision_hl:
        ld      a,CTX_PRECISION(iy)
        ld      l,a
        ld      a,CTX_PRECISION+1(iy)
        ld      h,a
        ret

__stdio_load_fmt_hl:
        ld      a,CTX_FMT(iy)
        ld      l,a
        ld      a,CTX_FMT+1(iy)
        ld      h,a
        ret

__stdio_store_fmt_hl::
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

__stdio_store_ap_hl::
        ld      a,l
        ld      CTX_AP(iy),a
        ld      a,h
        ld      CTX_AP+1(iy),a
        ret

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
        .if     __XCC_LIBC_LONG
        cp      #LEN_LONG
        jr      z,__stdio_load_uval_32
        .endif
        .if     __XCC_LIBC_LONGLONG
        cp      #LEN_LLONG
        jr      z,__stdio_load_uval_64
        .endif
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
        .if     __XCC_LIBC_LONG
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
        .endif
        .if     __XCC_LIBC_LONGLONG
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
        .endif

__stdio_uval_is_zero:
        ld      a,CTX_LENGTH(iy)
        .if     __XCC_LIBC_LONG
        cp      #LEN_LONG
        jr      z,__stdio_uval_is_zero_32
        .endif
        .if     __XCC_LIBC_LONGLONG
        cp      #LEN_LLONG
        jr      z,__stdio_uval_is_zero_64
        .endif
        ld      a,CTX_UVAL0(iy)
        ld      b,a
        ld      a,CTX_UVAL1(iy)
        or      b
        ret
        .if     __XCC_LIBC_LONG
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
        .endif
        .if     __XCC_LIBC_LONGLONG
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
        .endif

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
        .if     __XCC_LIBC_LONG
        cp      #LEN_LONG
        jr      z,__stdio_negate_uval_32
        .endif
        .if     __XCC_LIBC_LONGLONG
        cp      #LEN_LLONG
        jr      z,__stdio_negate_uval_64
        .endif
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
        .if     __XCC_LIBC_LONG
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
        .endif
        .if     __XCC_LIBC_LONGLONG
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
        .endif

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
        .if     __XCC_LIBC_LONG
        cp      #LEN_LONG
        jp      z,__stdio_divmod_uval_32
        .endif
        .if     __XCC_LIBC_LONGLONG
        cp      #LEN_LLONG
        jp      z,__stdio_divmod_uval_64
        .endif
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
        .if     __XCC_LIBC_LONG
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
        .endif
        .if     __XCC_LIBC_LONGLONG
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
        .endif

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

__stdio_store_count_ptr:
        call    __stdio_fetch_ptr_hl
        ld      a,h
        or      l
        ret     z
        ld      e,CTX_COUNT(iy)
        ld      d,CTX_COUNT+1(iy)
        ld      a,CTX_LENGTH(iy)
        .if     __XCC_LIBC_LONG
        cp      #LEN_LONG
        jr      z,__stdio_store_count_ptr_32
        .endif
        .if     __XCC_LIBC_LONGLONG
        cp      #LEN_LLONG
        jr      z,__stdio_store_count_ptr_64
        .endif
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret
        .if     __XCC_LIBC_LONG
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
        .endif
        .if     __XCC_LIBC_LONGLONG
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
        .endif

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
        cp      #'w'
        jr      z,__stdio_parse_length_w
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
__stdio_parse_length_w:
        inc     hl
        ld      a,(hl)
        cp      #'8'
        jr      z,__stdio_parse_length_w8
        cp      #'1'
        jr      z,__stdio_parse_length_w16
        cp      #'3'
        jr      z,__stdio_parse_length_w32
        cp      #'6'
        jr      z,__stdio_parse_length_w64
        ret
__stdio_parse_length_w8:
        inc     hl
        call    __stdio_store_fmt_hl
        ret
__stdio_parse_length_w16:
        inc     hl
        ld      a,(hl)
        cp      #'6'
        ret     nz
        inc     hl
        call    __stdio_store_fmt_hl
        ret
__stdio_parse_length_w32:
        inc     hl
        ld      a,(hl)
        cp      #'2'
        ret     nz
        inc     hl
        ld      a,#LEN_LONG
        ld      CTX_LENGTH(iy),a
        call    __stdio_store_fmt_hl
        ret
__stdio_parse_length_w64:
        inc     hl
        ld      a,(hl)
        cp      #'4'
        ret     nz
        inc     hl
        ld      a,#LEN_LLONG
        ld      CTX_LENGTH(iy),a
        call    __stdio_store_fmt_hl
        ret

__stdio_vformat::
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
        .if     __XCC_LIBC_STDIO_FLOAT
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
        .endif
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
        .if     __XCC_LIBC_LONG
        cp      #LEN_LONG
        jr      z,__stdio_vformat_signed_32
        .endif
        .if     __XCC_LIBC_LONGLONG
        cp      #LEN_LLONG
        jr      z,__stdio_vformat_signed_64
        .endif
        ld      a,CTX_UVAL1(iy)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      CTX_SIGN(iy),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
        .if     __XCC_LIBC_LONG
__stdio_vformat_signed_32:
        ld      a,CTX_UVAL3(iy)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      CTX_SIGN(iy),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
        .endif
        .if     __XCC_LIBC_LONGLONG
__stdio_vformat_signed_64:
        ld      a,CTX_UVAL7(iy)
        bit     7,a
        jr      z,__stdio_vformat_signed_nonneg
        ld      a,#'-'
        ld      CTX_SIGN(iy),a
        call    __stdio_negate_uval
        jr      __stdio_vformat_signed_ready
        .endif
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
        ld      a,CTX_FLAGS(iy)
        bit     3,a
        jr      z,__stdio_vformat_binary_build
        ld      a,CTX_VALUE_ZERO(iy)
        or      a
        jr      nz,__stdio_vformat_binary_build
        ld      a,#2
        ld      CTX_PREFIX_LEN(iy),a
        ld      a,#'0'
        ld      CTX_PREFIX_0(iy),a
        ld      a,CTX_UPPER(iy)
        or      a
        ld      a,#'b'
        jr      z,__stdio_vformat_binary_prefix_store
        ld      a,#'B'
__stdio_vformat_binary_prefix_store:
        ld      CTX_PREFIX_1(iy),a
__stdio_vformat_binary_build:
        call    __stdio_build_digits
        call    __stdio_emit_number
        jp      __stdio_vformat_loop

        .if     __XCC_LIBC_STDIO_FLOAT
__stdio_vformat_float:
        ; Render the variadic double into a temporary buffer, then emit it
        ; through the regular string-field path so width/alignment still work.
        ld      CTX_BASE(iy),a
        ld      hl,#-48
        add     hl,sp
        ld      sp,hl
        ld      hl,#0
        add     hl,sp
        call    __stdio_store_digits_ptr_hl
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

        ld      a,CTX_HAVE_PREC(iy)
        or      a
        jr      nz,__stdio_vformat_float_have_precision
        ld      hl,#6
        jr      __stdio_vformat_float_store_precision
__stdio_vformat_float_have_precision:
        call    __stdio_load_precision_hl
__stdio_vformat_float_store_precision:
        call    __stdio_store_digits_len_hl
        push    iy
        ld      a,CTX_UVAL6(iy)
        ld      l,a
        ld      a,CTX_UVAL7(iy)
        ld      h,a
        push    hl
        ld      a,CTX_UVAL4(iy)
        ld      l,a
        ld      a,CTX_UVAL5(iy)
        ld      h,a
        push    hl
        ld      a,CTX_UVAL2(iy)
        ld      l,a
        ld      a,CTX_UVAL3(iy)
        ld      h,a
        push    hl
        ld      a,CTX_UVAL0(iy)
        ld      l,a
        ld      a,CTX_UVAL1(iy)
        ld      h,a
        push    hl
        ld      a,CTX_FLAGS(iy)
        ld      l,a
        ld      h,#0
        push    hl
        ld      a,CTX_BASE(iy)
        ld      l,a
        ld      h,#0
        push    hl
        call    __stdio_load_digits_len_hl
        push    hl
        ld      hl,#48
        push    hl
        call    __stdio_load_digits_ptr_hl
        push    hl
        call    _stdio_format_double
        ld      hl,#18
        add     hl,sp
        ld      sp,hl
        pop     iy
        xor     a
        ld      CTX_HAVE_PREC(iy),a
        ld      hl,#0
        add     hl,sp
        call    __stdio_emit_string_field
        ld      hl,#48
        add     hl,sp
        ld      sp,hl
        jp      __stdio_vformat_loop
        .endif

__stdio_vformat_done:
        call    __stdio_finish_sink
        call    __stdio_load_count_hl
        ret
