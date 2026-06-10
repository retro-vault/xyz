        ;; wcstox_core.s
        ;;
        ;; Shared wide-string-to-integer parser for the target's single-byte
        ;; execution charset carried in 16-bit wchar_t code units.
        ;;
        ;; The accepted syntax mirrors strtox_core:
        ;;   - leading ASCII whitespace
        ;;   - optional sign
        ;;   - base-0 autodetection with 0 / 0x prefixes
        ;;   - digit runs for bases 2..36
        ;;
        ;; Any wchar_t whose high byte is non-zero terminates the parse the same
        ;; way a non-digit byte would in the narrow parser.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module wcstox_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __wcstox_core
        .globl  __wsx_acc
        .globl  __wsx_neg
        .globl  __wsx_ovf
        .globl  __wsx_any
        .globl  __wsx_negate

        .area   _DATA
__wsx_acc:: .ds 8
__wsx_tmp:  .ds 8
__wsx_neg:: .ds 1
__wsx_ovf:: .ds 1
__wsx_any:: .ds 1
__wsx_base: .ds 1
__wsx_dig:  .ds 1
__wsx_endp: .ds 2          ; wchar_t ** endptr (may be NULL)
__wsx_nptr: .ds 2          ; original nptr

        .area   _CODE

        ;; __wcstox_core
        ;; inputs:
        ;;   HL = nptr
        ;;   DE = endptr (wchar_t **)
        ;;   BC = base
        ;; outputs (statics):
        ;;   __wsx_acc[8]  parsed magnitude, little-endian
        ;;   __wsx_neg     1 when a '-' sign was present
        ;;   __wsx_ovf     1 when 64-bit overflow occurred
        ;;   __wsx_any     1 when at least one digit was consumed
        ;; and *endptr is updated to the first unparsed wchar_t (or nptr on
        ;; total matching failure).
__wcstox_core::
        ld      (__wsx_nptr),hl
        ld      (__wsx_endp),de
        ld      a,c
        ld      (__wsx_base),a

        ;; Reset accumulator and parser flags before walking the source.
        push    hl
        ld      hl,#__wsx_acc
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir
        xor     a
        ld      (__wsx_neg),a
        ld      (__wsx_ovf),a
        ld      (__wsx_any),a
        pop     hl

        ;; Skip leading ASCII whitespace only when the wide code unit is still
        ;; byte-range. Any non-ASCII wchar_t terminates the whitespace scan.
wsx_ws:
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_ws_done
        ld      a,e
        cp      #0x20
        jr      z,wsx_ws_next
        cp      #0x09
        jr      c,wsx_ws_done
        cp      #0x0e
        jr      nc,wsx_ws_done
wsx_ws_next:
        inc     hl
        inc     hl
        jr      wsx_ws
wsx_ws_done:

        ;; Optional sign.
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_base
        ld      a,e
        cp      #0x2b
        jr      z,wsx_sign_skip
        cp      #0x2d
        jr      nz,wsx_base
        ld      a,#1
        ld      (__wsx_neg),a
wsx_sign_skip:
        inc     hl
        inc     hl

wsx_base:
        ;; Base 0 follows the usual 0 / 0x rules. Base 16 also accepts an
        ;; optional 0x prefix when a real hex digit follows.
        ld      a,(__wsx_base)
        or      a
        jp      z,wsx_base0
        cp      #16
        jp      nz,wsx_base_ok

        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jp      nz,wsx_base_ok
        ld      a,e
        cp      #0x30
        jp      nz,wsx_base_ok

        push    hl
        inc     hl
        inc     hl
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_b16_no
        ld      a,e
        cp      #0x78
        jr      z,wsx_b16_x
        cp      #0x58
        jr      nz,wsx_b16_no
wsx_b16_x:
        inc     hl
        inc     hl
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_b16_no
        ld      a,e
        call    wsx_digitval
        cp      #16
        jr      nc,wsx_b16_no
        pop     bc
        jr      wsx_loop
wsx_b16_no:
        pop     hl
        jr      wsx_base_ok

wsx_base0:
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_base0_dec
        ld      a,e
        cp      #0x30
        jr      nz,wsx_base0_dec

        push    hl
        inc     hl
        inc     hl
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_b0_oct
        ld      a,e
        cp      #0x78
        jr      z,wsx_b0_x
        cp      #0x58
        jr      nz,wsx_b0_oct
wsx_b0_x:
        inc     hl
        inc     hl
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_b0_oct
        ld      a,e
        call    wsx_digitval
        cp      #16
        jr      nc,wsx_b0_oct
        pop     bc
        ld      a,#16
        ld      (__wsx_base),a
        jr      wsx_loop
wsx_b0_oct:
        pop     hl
        ld      a,#8
        ld      (__wsx_base),a
        jr      wsx_loop
wsx_base0_dec:
        ld      a,#10
        ld      (__wsx_base),a
        jr      wsx_loop

wsx_base_ok:
        ld      a,(__wsx_base)
        cp      #2
        jr      c,wsx_done
        cp      #37
        jr      nc,wsx_done

wsx_loop:
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wsx_loop_end
        ld      a,e
        call    wsx_digitval
        ld      b,a
        ld      a,(__wsx_base)
        cp      b
        jr      c,wsx_loop_end
        jr      z,wsx_loop_end

        ld      a,#1
        ld      (__wsx_any),a
        ld      a,(__wsx_ovf)
        or      a
        jr      nz,wsx_loop_adv
        ld      a,b
        ld      (__wsx_dig),a
        push    hl
        call    wsx_accum
        pop     hl
wsx_loop_adv:
        inc     hl
        inc     hl
        jr      wsx_loop

wsx_loop_end:
        ld      a,(__wsx_any)
        or      a
        jr      z,wsx_done
        ld      a,(__wsx_endp)
        ld      c,a
        ld      a,(__wsx_endp + 1)
        ld      b,a
        or      c
        ret     z
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        ret

wsx_done:
        ld      a,(__wsx_endp)
        ld      c,a
        ld      a,(__wsx_endp + 1)
        ld      b,a
        or      c
        ret     z
        ld      a,(__wsx_nptr)
        ld      (bc),a
        inc     bc
        ld      a,(__wsx_nptr + 1)
        ld      (bc),a
        ret

        ;; A = ASCII byte -> A = 0..35, or 0xFF for non-digit.
wsx_digitval:
        cp      #0x30
        jr      c,wsx_dv_bad
        cp      #0x3a
        jr      c,wsx_dv_dig
        cp      #0x41
        jr      c,wsx_dv_bad
        cp      #0x5b
        jr      c,wsx_dv_up
        cp      #0x61
        jr      c,wsx_dv_bad
        cp      #0x7b
        jr      c,wsx_dv_lo
wsx_dv_bad:
        ld      a,#0xff
        ret
wsx_dv_dig:
        sub     #0x30
        ret
wsx_dv_up:
        sub     #0x37
        ret
wsx_dv_lo:
        sub     #0x57
        ret

        ;; __wsx_acc = __wsx_acc * base + digit
wsx_accum:
        ld      hl,#__wsx_tmp
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir

        ld      a,(__wsx_base)
        ld      b,a
wsx_mul:
        push    bc
        ld      hl,#__wsx_tmp
        ld      de,#__wsx_acc
        call    wsx_add64
        jr      nc,wsx_mul_nc
        ld      a,#1
        ld      (__wsx_ovf),a
wsx_mul_nc:
        pop     bc
        djnz    wsx_mul

        ld      hl,#__wsx_tmp
        ld      a,(__wsx_dig)
        add     a,(hl)
        ld      (hl),a
        ld      b,#7
wsx_dcarry:
        inc     hl
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        djnz    wsx_dcarry
        jr      nc,wsx_acc_copy
        ld      a,#1
        ld      (__wsx_ovf),a
wsx_acc_copy:
        ld      hl,#__wsx_tmp
        ld      de,#__wsx_acc
        ld      bc,#8
        ldir
        ret

wsx_add64:
        or      a
        ld      b,#8
wsx_add64_l:
        ld      a,(de)
        adc     a,(hl)
        ld      (hl),a
        inc     hl
        inc     de
        djnz    wsx_add64_l
        ret

        ;; Two's-complement negate __wsx_acc[8] in place.
__wsx_negate::
        ld      hl,#__wsx_acc
        ld      b,#8
wsxn_cpl:
        ld      a,(hl)
        cpl
        ld      (hl),a
        inc     hl
        djnz    wsxn_cpl
        ld      hl,#__wsx_acc
        ld      b,#8
        scf
wsxn_inc:
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        inc     hl
        djnz    wsxn_inc
        ret
