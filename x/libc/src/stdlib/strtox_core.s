        ;; strtox_core.s
        ;;
        ;; Shared string->integer parser for strtol/strtoul/strtoll/strtoull.
        ;; The parser now writes its 64-bit magnitude into a caller-provided
        ;; buffer so the libc stays reentrant: no module-global scratch remains.
        ;;
        ;; Calling convention:
        ;;   HL = nptr
        ;;   DE = endptr (char **, may be NULL)
        ;;   BC = base
        ;;   IY = destination buffer for the 8-byte little-endian magnitude
        ;;
        ;; Return flags in A:
        ;;   bit 0 = at least one digit consumed
        ;;   bit 1 = leading '-' sign present
        ;;   bit 2 = 64-bit overflow occurred while accumulating
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih





        .module strtox_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __strtox_core

SX_BASE         .equ -2                ; base (low byte only)
SX_DIG          .equ -1                ; current digit
SX_ENDP         .equ -5                ; endptr
SX_FLAGS        .equ -3                ; parser flags
SX_FLAG_ANY     .equ 0x01
SX_FLAG_NEG     .equ 0x02
SX_FLAG_OVF     .equ 0x04
SX_NPTR         .equ -7                ; original nptr
SX_TMP          .equ -15               ; 8-byte local tmp at -15..-8

        .area   _CODE
__strtox_core::
        ld      a,c
        ld      c,l
        ld      b,h
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-15
        add     hl,sp
        ld      sp,hl
        ld      l,c
        ld      h,b

        ld      SX_NPTR(ix),l
        ld      SX_NPTR + 1(ix),h
        ld      SX_ENDP(ix),e
        ld      SX_ENDP + 1(ix),d
        ld      SX_BASE(ix),a
        xor     a
        ld      SX_FLAGS(ix),a
        ld      SX_DIG(ix),a

        ;; Zero the caller-provided 64-bit accumulator.
        push    iy
        pop     hl
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir
        ld      l,SX_NPTR(ix)
        ld      h,SX_NPTR + 1(ix)

sx_ws:
        ld      a,(hl)
        cp      #0x20
        jr      z,sx_ws_next
        cp      #0x09
        jr      c,sx_ws_done
        cp      #0x0e
        jr      nc,sx_ws_done
sx_ws_next:
        inc     hl
        jr      sx_ws
sx_ws_done:

        ld      a,(hl)
        cp      #0x2b
        jr      z,sx_sign_skip
        cp      #0x2d
        jr      nz,sx_base
        ld      a,SX_FLAGS(ix)
        or      #SX_FLAG_NEG
        ld      SX_FLAGS(ix),a
sx_sign_skip:
        inc     hl

sx_base:
        ld      a,SX_BASE(ix)
        or      a
        jr      z,sx_base0
        cp      #16
        jr      nz,sx_base_ok
        ld      a,(hl)
        cp      #0x30
        jr      nz,sx_base_ok
        push    hl
        inc     hl
        ld      a,(hl)
        cp      #0x78
        jr      z,sx_b16_x
        cp      #0x58
        jr      nz,sx_b16_no
sx_b16_x:
        inc     hl
        ld      a,(hl)
        call    sx_digitval
        cp      #16
        jr      nc,sx_b16_no
        pop     bc
        jr      sx_loop
sx_b16_no:
        pop     hl
        jr      sx_base_ok

sx_base0:
        ld      a,(hl)
        cp      #0x30
        jr      nz,sx_base0_dec
        push    hl
        inc     hl
        ld      a,(hl)
        cp      #0x78
        jr      z,sx_b0_x
        cp      #0x58
        jr      nz,sx_b0_oct
sx_b0_x:
        inc     hl
        ld      a,(hl)
        call    sx_digitval
        cp      #16
        jr      nc,sx_b0_oct
        pop     bc
        ld      a,#16
        ld      SX_BASE(ix),a
        jr      sx_loop
sx_b0_oct:
        pop     hl
        ld      a,#8
        ld      SX_BASE(ix),a
        jr      sx_loop
sx_base0_dec:
        ld      a,#10
        ld      SX_BASE(ix),a
        jr      sx_loop

sx_base_ok:
        ld      a,SX_BASE(ix)
        cp      #2
        jr      c,sx_done
        cp      #37
        jr      nc,sx_done

sx_loop:
        ld      a,(hl)
        call    sx_digitval
        ld      b,a
        ld      a,SX_BASE(ix)
        cp      b
        jr      c,sx_loop_end
        jr      z,sx_loop_end
        ld      a,SX_FLAGS(ix)
        or      #SX_FLAG_ANY
        ld      SX_FLAGS(ix),a
        bit     2,a
        jr      nz,sx_loop_adv
        ld      a,b
        ld      SX_DIG(ix),a
        push    hl
        call    sx_accum
        pop     hl
sx_loop_adv:
        inc     hl
        jr      sx_loop

sx_loop_end:
        ld      a,SX_FLAGS(ix)
        bit     0,a
        jr      z,sx_done
        ld      c,SX_ENDP(ix)
        ld      b,SX_ENDP + 1(ix)
        ld      a,b
        or      c
        jr      z,sx_return
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        jr      sx_return

sx_done:
        ld      c,SX_ENDP(ix)
        ld      b,SX_ENDP + 1(ix)
        ld      a,b
        or      c
        jr      z,sx_return
        ld      a,SX_NPTR(ix)
        ld      (bc),a
        inc     bc
        ld      a,SX_NPTR + 1(ix)
        ld      (bc),a

sx_return:
        ld      a,SX_FLAGS(ix)
        ld      sp,ix
        pop     ix
        ret

sx_digitval:
        cp      #0x30
        jr      c,sx_dv_bad
        cp      #0x3a
        jr      c,sx_dv_dig
        cp      #0x41
        jr      c,sx_dv_bad
        cp      #0x5b
        jr      c,sx_dv_up
        cp      #0x61
        jr      c,sx_dv_bad
        cp      #0x7b
        jr      c,sx_dv_lo
sx_dv_bad:
        ld      a,#0xff
        ret
sx_dv_dig:
        sub     #0x30
        ret
sx_dv_up:
        sub     #0x37
        ret
sx_dv_lo:
        sub     #0x57
        ret

        ;; Multiply the current magnitude by BASE and add DIG, both held in the
        ;; local frame. IY still points at the 8-byte destination buffer.
sx_accum:
        push    ix
        pop     hl
        ld      bc,#SX_TMP
        add     hl,bc
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir

        ld      a,SX_BASE(ix)
        ld      b,a
sx_mul:
        push    bc
        push    ix
        pop     hl
        ld      bc,#SX_TMP
        add     hl,bc
        push    iy
        pop     de
        call    sx_add64
        jr      nc,sx_mul_nc
        ld      a,SX_FLAGS(ix)
        or      #SX_FLAG_OVF
        ld      SX_FLAGS(ix),a
sx_mul_nc:
        pop     bc
        djnz    sx_mul

        push    ix
        pop     hl
        ld      bc,#SX_TMP
        add     hl,bc
        ld      a,SX_DIG(ix)
        add     a,(hl)
        ld      (hl),a
        ld      b,#7
sx_dcarry:
        inc     hl
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        djnz    sx_dcarry
        jr      nc,sx_acc_copy
        ld      a,SX_FLAGS(ix)
        or      #SX_FLAG_OVF
        ld      SX_FLAGS(ix),a
sx_acc_copy:
        push    ix
        pop     hl
        ld      bc,#SX_TMP
        add     hl,bc
        push    iy
        pop     de
        ld      bc,#8
        ldir
        ret

        ;; (HL)[8] += (DE)[8], carry set on overflow out of bit 63.
sx_add64:
        or      a
        ld      b,#8
sx_add64_l:
        ld      a,(de)
        adc     a,(hl)
        ld      (hl),a
        inc     hl
        inc     de
        djnz    sx_add64_l
        ret

        ;; Two's-complement negate the 8-byte little-endian buffer at HL.
