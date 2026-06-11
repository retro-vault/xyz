        ;; wcstox_core.s
        ;;
        ;; Wide-string-to-integer parser matching the narrow strtox_core
        ;; contract, but walking 16-bit wchar_t code units that carry the
        ;; single-byte execution charset in their low byte.
        ;;
        ;; Calling convention:
        ;;   HL = nptr
        ;;   DE = endptr (wchar_t **, may be NULL)
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

        .module wcstox_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __wcstox_core
        .globl  __wsx_negate

WSX_FLAG_ANY    .equ 0x01
WSX_FLAG_NEG    .equ 0x02
WSX_FLAG_OVF    .equ 0x04

WSX_TMP         .equ -15
WSX_NPTR        .equ -7
WSX_ENDP        .equ -5
WSX_FLAGS       .equ -3
WSX_BASE        .equ -2
WSX_DIG         .equ -1

        .area   _CODE

__wcstox_core::
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

        ld      WSX_NPTR(ix),l
        ld      WSX_NPTR + 1(ix),h
        ld      WSX_ENDP(ix),e
        ld      WSX_ENDP + 1(ix),d
        ld      WSX_BASE(ix),a
        xor     a
        ld      WSX_FLAGS(ix),a
        ld      WSX_DIG(ix),a

        push    iy
        pop     hl
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir
        ld      l,WSX_NPTR(ix)
        ld      h,WSX_NPTR + 1(ix)

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
        ld      a,WSX_FLAGS(ix)
        or      #WSX_FLAG_NEG
        ld      WSX_FLAGS(ix),a
wsx_sign_skip:
        inc     hl
        inc     hl

wsx_base:
        ld      a,WSX_BASE(ix)
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
        ld      WSX_BASE(ix),a
        jr      wsx_loop
wsx_b0_oct:
        pop     hl
        ld      a,#8
        ld      WSX_BASE(ix),a
        jr      wsx_loop
wsx_base0_dec:
        ld      a,#10
        ld      WSX_BASE(ix),a
        jr      wsx_loop

wsx_base_ok:
        ld      a,WSX_BASE(ix)
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
        ld      a,WSX_BASE(ix)
        cp      b
        jr      c,wsx_loop_end
        jr      z,wsx_loop_end
        ld      a,WSX_FLAGS(ix)
        or      #WSX_FLAG_ANY
        ld      WSX_FLAGS(ix),a
        bit     2,a
        jr      nz,wsx_loop_adv
        ld      a,b
        ld      WSX_DIG(ix),a
        push    hl
        call    wsx_accum
        pop     hl
wsx_loop_adv:
        inc     hl
        inc     hl
        jr      wsx_loop

wsx_loop_end:
        ld      a,WSX_FLAGS(ix)
        bit     0,a
        jr      z,wsx_done
        ld      c,WSX_ENDP(ix)
        ld      b,WSX_ENDP + 1(ix)
        ld      a,b
        or      c
        jr      z,wsx_return
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        jr      wsx_return

wsx_done:
        ld      c,WSX_ENDP(ix)
        ld      b,WSX_ENDP + 1(ix)
        ld      a,b
        or      c
        jr      z,wsx_return
        ld      a,WSX_NPTR(ix)
        ld      (bc),a
        inc     bc
        ld      a,WSX_NPTR + 1(ix)
        ld      (bc),a

wsx_return:
        ld      a,WSX_FLAGS(ix)
        ld      sp,ix
        pop     ix
        ret

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

wsx_accum:
        push    ix
        pop     hl
        ld      bc,#WSX_TMP
        add     hl,bc
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir

        ld      a,WSX_BASE(ix)
        ld      b,a
wsx_mul:
        push    bc
        push    ix
        pop     hl
        ld      bc,#WSX_TMP
        add     hl,bc
        push    iy
        pop     de
        call    wsx_add64
        jr      nc,wsx_mul_nc
        ld      a,WSX_FLAGS(ix)
        or      #WSX_FLAG_OVF
        ld      WSX_FLAGS(ix),a
wsx_mul_nc:
        pop     bc
        djnz    wsx_mul

        push    ix
        pop     hl
        ld      bc,#WSX_TMP
        add     hl,bc
        ld      a,WSX_DIG(ix)
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
        ld      a,WSX_FLAGS(ix)
        or      #WSX_FLAG_OVF
        ld      WSX_FLAGS(ix),a
wsx_acc_copy:
        push    ix
        pop     hl
        ld      bc,#WSX_TMP
        add     hl,bc
        push    iy
        pop     de
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

__wsx_negate::
        ld      b,#8
wsxn_cpl:
        ld      a,(hl)
        cpl
        ld      (hl),a
        inc     hl
        djnz    wsxn_cpl
        ld      bc,#-8
        add     hl,bc
        ld      b,#8
        scf
wsxn_inc:
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        inc     hl
        djnz    wsxn_inc
        ret
