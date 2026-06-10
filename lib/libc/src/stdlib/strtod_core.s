        ;; strtod_core.s
        ;;
        ;; Shared decimal string-to-floating parser for atof/strtof/strtod/strtold.
        ;; The parser handles:
        ;;   - leading ASCII whitespace
        ;;   - optional '+' / '-'
        ;;   - decimal digits with optional '.'
        ;;   - optional scientific exponent 'e' / 'E'
        ;;
        ;; The numeric core keeps up to 18 significant decimal digits in an
        ;; unsigned 64-bit accumulator, tracks how many digits were skipped once
        ;; that budget is exhausted, and then forms:
        ;;
        ;;   value ~= accumulator * 10^(explicit_exp + skipped - frac_digits)
        ;;
        ;; The final scaling is performed in the existing double runtime so the
        ;; public wrappers only need ABI glue.
        ;;
        ;; This is a pragmatic libc parser, not a full C23 hexadecimal-float /
        ;; locale-sensitive implementation yet.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module strtod_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __strtod_core
        .globl  __errno_value
        .globl  ___ull2db
        .globl  __dbmul
        .globl  __dbdiv
        .globl  __dbneg
        .globl  __db_zero

        .area   _DATA
__sfp_acc:      .ds 8
__sfp_tmp:      .ds 8
__sfp_nptr:     .ds 2
__sfp_endp:     .ds 2
__sfp_expmark:  .ds 2
__sfp_frac:     .ds 2
__sfp_skip:     .ds 2
__sfp_exp:      .ds 2
__sfp_any:      .ds 1
__sfp_neg:      .ds 1
__sfp_dot:      .ds 1
__sfp_col:      .ds 1

        .area   _CODE

        ;; __strtod_core
        ;; inputs:  HL = nptr, DE = endptr
        ;; outputs: DE:HL:DE':HL' = parsed double
        ;;          *endptr updated if non-NULL
        ;; clobbers: af, bc, de, hl, ix, de', hl'
__strtod_core::
        ld      (__sfp_nptr),hl
        ld      a,e
        ld      (__sfp_endp),a
        ld      a,d
        ld      (__sfp_endp + 1),a
        call    sfp_clear_state
        call    sfp_skip_ws

        ;; Optional leading sign is kept separately so zero, underflow, and
        ;; scaled results can all reuse the same final negate step.
        ld      a,(hl)
        cp      #0x2b                   ; '+'
        jr      z,sfp_skip_sign
        cp      #0x2d                   ; '-'
        jr      nz,sfp_parse
        ld      a,#1
        ld      (__sfp_neg),a
sfp_skip_sign:
        inc     hl

sfp_parse:
        ld      a,(hl)
        cp      #0x2e                   ; '.'
        jr      z,sfp_decimal
        call    sfp_digit
        cp      #10
        jr      nc,sfp_parse_end
        ld      b,a                     ; digit
        ld      a,#1
        ld      (__sfp_any),a
        ld      a,(__sfp_dot)
        or      a
        call    nz,sfp_inc_frac
        ld      a,b
        or      a
        jr      nz,sfp_collect_digit
        ld      a,(__sfp_col)
        or      a
        jr      z,sfp_parse_advance
sfp_collect_digit:
        ld      a,(__sfp_col)
        cp      #18
        jr      nc,sfp_skip_digit
        ld      a,b
        push    hl
        call    sfp_mul10add
        pop     hl
        ld      a,(__sfp_col)
        inc     a
        ld      (__sfp_col),a
        jr      sfp_parse_advance
sfp_skip_digit:
        call    sfp_inc_skip
sfp_parse_advance:
        inc     hl
        jr      sfp_parse

sfp_decimal:
        ld      a,(__sfp_dot)
        or      a
        jr      nz,sfp_parse_end
        ld      a,#1
        ld      (__sfp_dot),a
        inc     hl
        jr      sfp_parse

sfp_parse_end:
        ld      a,(__sfp_any)
        or      a
        jp      z,sfp_fail

        ;; Optional scientific exponent. If the exponent marker is not followed
        ;; by a decimal digit sequence, it is not consumed.
        ld      a,(hl)
        cp      #0x65                   ; 'e'
        jr      z,sfp_exp_try
        cp      #0x45                   ; 'E'
        jr      nz,sfp_finish_parse
sfp_exp_try:
        ld      (__sfp_expmark),hl
        inc     hl
        xor     a
        ld      (__sfp_exp),a
        ld      (__sfp_exp + 1),a
        ld      b,#0                    ; exponent sign: 0=+, 1=-
        ld      a,(hl)
        cp      #0x2b                   ; '+'
        jr      z,sfp_exp_after_sign
        cp      #0x2d                   ; '-'
        jr      nz,sfp_exp_digits
        ld      b,#1
sfp_exp_after_sign:
        inc     hl
sfp_exp_digits:
        ld      c,#0                    ; exponent had at least one digit
sfp_exp_loop:
        ld      a,(hl)
        call    sfp_digit
        cp      #10
        jr      nc,sfp_exp_done
        ld      c,#1
        push    hl
        call    sfp_exp_mul10add
        pop     hl
        inc     hl
        jr      sfp_exp_loop
sfp_exp_done:
        ld      a,c
        or      a
        jr      nz,sfp_exp_valid
        ld      hl,(__sfp_expmark)
        jr      sfp_finish_parse
sfp_exp_valid:
        ld      a,b
        or      a
        jr      z,sfp_finish_parse
        call    sfp_neg_exp

sfp_finish_parse:
        call    sfp_store_end_hl
        call    sfp_acc_is_zero
        jr      z,sfp_return_zero

        ;; final_exp = explicit_exp + skipped_digits - frac_digits
        ld      hl,(__sfp_exp)
        push    hl
        ld      hl,(__sfp_skip)
        ex      de,hl
        pop     hl
        add     hl,de
        push    hl
        ld      hl,(__sfp_frac)
        ex      de,hl
        pop     hl
        or      a
        sbc     hl,de
        push    hl                      ; save decimal exponent

        ld      hl,(__sfp_acc)
        ex      de,hl
        ld      hl,(__sfp_acc + 2)
        exx
        ld      hl,(__sfp_acc + 4)
        ex      de,hl
        ld      hl,(__sfp_acc + 6)
        exx
        call    ___ull2db

        pop     hl
        call    sfp_scale_result
        call    sfp_apply_sign
        ret

sfp_return_zero:
        call    __db_zero
        call    sfp_apply_sign
        ret

sfp_fail:
        ld      hl,(__sfp_nptr)
        call    sfp_store_end_hl
        jp      __db_zero

        ;; Clears the shared parser state before each public call.
sfp_clear_state:
        push    hl
        xor     a
        ld      (__sfp_frac),a
        ld      (__sfp_frac + 1),a
        ld      (__sfp_skip),a
        ld      (__sfp_skip + 1),a
        ld      (__sfp_exp),a
        ld      (__sfp_exp + 1),a
        ld      (__sfp_any),a
        ld      (__sfp_neg),a
        ld      (__sfp_dot),a
        ld      (__sfp_col),a
        ld      hl,#__sfp_acc
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir
        pop     hl
        ret

        ;; Advances HL past the ASCII whitespace accepted by the integer parser.
sfp_skip_ws:
        ld      a,(hl)
        cp      #0x20
        jr      z,sfp_ws_next
        cp      #0x09
        jr      c,sfp_skip_ws_done
        cp      #0x0e
        jr      nc,sfp_skip_ws_done
sfp_ws_next:
        inc     hl
        jr      sfp_skip_ws
sfp_skip_ws_done:
        ret

        ;; Returns A = 0..9 for decimal digits, 0xFF otherwise.
sfp_digit:
        cp      #0x30
        jr      c,sfp_digit_bad
        cp      #0x3a
        jr      nc,sfp_digit_bad
        sub     #0x30
        ret
sfp_digit_bad:
        ld      a,#0xff
        ret

        ;; frac_digits++
sfp_inc_frac:
        ld      a,(__sfp_frac)
        inc     a
        ld      (__sfp_frac),a
        ret     nz
        ld      a,(__sfp_frac + 1)
        inc     a
        ld      (__sfp_frac + 1),a
        ret

        ;; skipped_digits++
sfp_inc_skip:
        ld      a,(__sfp_skip)
        inc     a
        ld      (__sfp_skip),a
        ret     nz
        ld      a,(__sfp_skip + 1)
        inc     a
        ld      (__sfp_skip + 1),a
        ret

        ;; exponent = exponent * 10 + digit(A), with a loose saturation at 999.
sfp_exp_mul10add:
        ld      c,a                     ; save digit
        ld      hl,(__sfp_exp)
        ld      a,h
        cp      #0x03
        jr      c,sfp_exp_mul_do
        jr      nz,sfp_exp_sat
        ld      a,l
        cp      #0xe7                   ; 999
        jr      nc,sfp_exp_sat
sfp_exp_mul_do:
        add     hl,hl
        push    hl
        add     hl,hl
        add     hl,hl
        pop     de
        add     hl,de                   ; old*10
        ld      a,l
        add     a,c
        ld      l,a
        jr      nc,sfp_exp_store
        inc     h
sfp_exp_store:
        ld      (__sfp_exp),hl
        ret
sfp_exp_sat:
        ld      hl,#999
        ld      (__sfp_exp),hl
        ret

        ;; exponent = -exponent
sfp_neg_exp:
        ld      hl,(__sfp_exp)
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl
        ld      (__sfp_exp),hl
        ret

        ;; acc = acc * 10 + digit(A).  Since the parser only keeps the first 18
        ;; significant digits, this path never overflows uint64.
sfp_mul10add:
        ld      (__sfp_tmp),a           ; tmp[0] starts with the new digit
        xor     a
        ld      (__sfp_tmp + 1),a
        ld      (__sfp_tmp + 2),a
        ld      (__sfp_tmp + 3),a
        ld      (__sfp_tmp + 4),a
        ld      (__sfp_tmp + 5),a
        ld      (__sfp_tmp + 6),a
        ld      (__sfp_tmp + 7),a
        ld      b,#10
sfp_mul_loop:
        push    bc
        ld      hl,#__sfp_tmp
        ld      de,#__sfp_acc
        call    sfp_add64
        pop     bc
        djnz    sfp_mul_loop
        ld      hl,#__sfp_tmp
        ld      de,#__sfp_acc
        ld      bc,#8
        ldir
        ret

        ;; tmp[8] += acc[8]
sfp_add64:
        xor     a
        ld      b,#8
sfp_add64_loop:
        ld      a,(de)
        adc     a,(hl)
        ld      (hl),a
        inc     de
        inc     hl
        djnz    sfp_add64_loop
        ret

        ;; Z if acc == 0, NZ otherwise.
sfp_acc_is_zero:
        ld      a,(__sfp_acc)
        ld      b,a
        ld      a,(__sfp_acc + 1)
        or      b
        ld      b,a
        ld      a,(__sfp_acc + 2)
        or      b
        ld      b,a
        ld      a,(__sfp_acc + 3)
        or      b
        ld      b,a
        ld      a,(__sfp_acc + 4)
        or      b
        ld      b,a
        ld      a,(__sfp_acc + 5)
        or      b
        ld      b,a
        ld      a,(__sfp_acc + 6)
        or      b
        ld      b,a
        ld      a,(__sfp_acc + 7)
        or      b
        ret

        ;; Stores HL into *endptr if endptr != NULL.
sfp_store_end_hl:
        ld      a,(__sfp_endp)
        ld      c,a
        ld      a,(__sfp_endp + 1)
        ld      b,a
        or      c
        ret     z
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        ret

        ;; Applies the remembered leading '-' sign to the current double result.
sfp_apply_sign:
        ld      a,(__sfp_neg)
        or      a
        ret     z
        jp      __dbneg

        ;; Scales the current double result by 10^HL.
        ;; Positive exponents multiply by 10.0, negative exponents divide by 10.0.
        ;; Extremely large decimal exponents are clamped to infinity / signed zero
        ;; and raise ERANGE.
sfp_scale_result:
        ld      a,h
        or      l
        ret     z
        bit     7,h
        jr      z,sfp_scale_pos

        ;; Negative exponent: if |exp| > 350, underflow to signed zero.
        call    sfp_neg_hl
        ld      (__sfp_exp),hl
        ld      a,h
        cp      #0x01
        jr      c,sfp_scale_neg_loop
        jr      nz,sfp_scale_under
        ld      a,l
        cp      #0x5f                   ; 351
        jr      nc,sfp_scale_under
sfp_scale_neg_loop:
        ld      hl,(__sfp_exp)
        ld      a,h
        or      l
        ret     z
        call    sfp_div10
        ld      hl,(__sfp_exp)
        dec     hl
        ld      (__sfp_exp),hl
        jr      sfp_scale_neg_loop

sfp_scale_pos:
        ;; Positive exponent: if exp > 308, overflow to infinity.
        ld      (__sfp_exp),hl
        ld      a,h
        cp      #0x01
        jr      c,sfp_scale_pos_loop
        jr      nz,sfp_scale_over
        ld      a,l
        cp      #0x35                   ; 309
        jr      nc,sfp_scale_over
sfp_scale_pos_loop:
        ld      hl,(__sfp_exp)
        ld      a,h
        or      l
        ret     z
        call    sfp_mul10
        ld      hl,(__sfp_exp)
        dec     hl
        ld      (__sfp_exp),hl
        jr      sfp_scale_pos_loop

sfp_scale_over:
        ld      hl,#34                  ; ERANGE
        ld      (__errno_value),hl
        jp      sfp_set_inf

sfp_scale_under:
        ld      hl,#34                  ; ERANGE
        ld      (__errno_value),hl
        jp      __db_zero

        ;; HL = -HL
sfp_neg_hl:
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl
        ret

        ;; Helper call wrappers for *= 10.0 and /= 10.0.
sfp_mul10:
        ld      hl,#0x4024
        push    hl
        ld      hl,#0x0000
        push    hl
        push    hl
        push    hl
        call    __dbmul
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        ret

sfp_div10:
        ld      hl,#0x4024
        push    hl
        ld      hl,#0x0000
        push    hl
        push    hl
        push    hl
        call    __dbdiv
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        ret

        ;; Returns +infinity in DE:HL:DE':HL'.
sfp_set_inf:
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x7ff0
        exx
        ret
