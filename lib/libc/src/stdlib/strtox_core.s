        ; strtox_core.s
        ;
        ; Shared string->integer parser for strtol/strtoul/strtoll/strtoull.
        ; Parses an optional sign, an optional 0x/0 base prefix, and a run of
        ; base-N digits into a 64-bit unsigned accumulator, tracking sign,
        ; overflow and whether any digit was seen.  The width-specific wrappers
        ; apply per-type range limits and signs.
        ;
        ; Outputs (statics):
        ;   __sx_acc[8]  unsigned magnitude (little-endian)
        ;   __sx_neg     1 if a '-' sign was present
        ;   __sx_ovf     1 if the value overflowed 64 bits
        ;   __sx_any     1 if at least one valid digit was consumed
        ; and *endptr is set (to the first unparsed char, or to nptr on failure).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strtox_core
        .optsdcc -mz80 sdcccall(1)
        .globl  __strtox_core
        .globl  __sx_acc
        .globl  __sx_neg
        .globl  __sx_ovf
        .globl  __sx_any
        .globl  __sx_negate

        .area   _DATA
__sx_acc::  .ds 8
__sx_tmp:   .ds 8
__sx_neg::  .ds 1
__sx_ovf::  .ds 1
__sx_any::  .ds 1
__sx_base:  .ds 1
__sx_dig:   .ds 1
__sx_endp:  .ds 2          ; char ** endptr (may be 0)
__sx_nptr:  .ds 2          ; original nptr

        .area   _CODE

        ; __strtox_core
        ; inputs: HL = nptr, DE = endptr (char**), BC = base
        ; clobbers: everything; results in statics
__strtox_core::
        ld      (__sx_nptr),hl
        ld      (__sx_endp),de
        ld      a,c
        ld      (__sx_base),a           ; base (low byte; 0..36)
        ; clear acc[8], neg/ovf/any
        push    hl
        ld      hl,#__sx_acc
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir                            ; acc = 0
        xor     a
        ld      (__sx_neg),a
        ld      (__sx_ovf),a
        ld      (__sx_any),a
        pop     hl                      ; HL = p = nptr
        ; --- skip whitespace ---
sx_ws:
        ld      a,(hl)
        cp      #0x20                   ; space
        jr      z,sx_ws_next
        cp      #0x09
        jr      c,sx_ws_done            ; < TAB -> not space
        cp      #0x0e
        jr      nc,sx_ws_done           ; > CR  -> not space
sx_ws_next:
        inc     hl
        jr      sx_ws
sx_ws_done:
        ; --- sign ---
        ld      a,(hl)
        cp      #0x2b                   ; '+'
        jr      z,sx_sign_skip
        cp      #0x2d                   ; '-'
        jr      nz,sx_base
        ld      a,#1
        ld      (__sx_neg),a
sx_sign_skip:
        inc     hl
sx_base:
        ; --- base detection / prefix ---
        ld      a,(__sx_base)
        or      a
        jr      z,sx_base0              ; base == 0 : auto
        cp      #16
        jr      nz,sx_base_ok
        ; base 16: optional 0x prefix
        ld      a,(hl)
        cp      #0x30                   ; '0'
        jr      nz,sx_base_ok
        push    hl
        inc     hl
        ld      a,(hl)
        cp      #0x78                   ; 'x'
        jr      z,sx_b16_x
        cp      #0x58                   ; 'X'
        jr      nz,sx_b16_no
sx_b16_x:
        inc     hl
        ld      a,(hl)
        call    sx_digitval
        cp      #16
        jr      nc,sx_b16_no            ; not a hex digit after 0x
        pop     bc                      ; discard saved
        jr      sx_loop                 ; HL already past "0x"
sx_b16_no:
        pop     hl                      ; restore
        jr      sx_base_ok
sx_base0:
        ld      a,(hl)
        cp      #0x30                   ; '0' ?
        jr      nz,sx_base0_dec
        ; leading 0: check for 0x
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
        jr      nc,sx_b0_oct            ; "0x" not followed by hex -> octal
        pop     bc
        ld      a,#16
        ld      (__sx_base),a
        jr      sx_loop                 ; HL past "0x"
sx_b0_oct:
        pop     hl
        ld      a,#8
        ld      (__sx_base),a
        jr      sx_loop
sx_base0_dec:
        ld      a,#10
        ld      (__sx_base),a
        jr      sx_loop
sx_base_ok:
        ; validate 2..36
        ld      a,(__sx_base)
        cp      #2
        jr      c,sx_done               ; < 2 invalid -> no digits
        cp      #37
        jr      nc,sx_done              ; > 36 invalid
sx_loop:
        ld      a,(hl)
        call    sx_digitval             ; A = digit or >=37 (0xFF)
        ld      b,a
        ld      a,(__sx_base)
        cp      b
        jr      c,sx_loop_end           ; base <= digit -> stop  (digit invalid)
        jr      z,sx_loop_end           ; base == digit -> stop
        ; valid digit in B
        ld      a,#1
        ld      (__sx_any),a
        ld      a,(__sx_ovf)
        or      a
        jr      nz,sx_loop_adv          ; already overflowed: just consume
        ld      a,b
        ld      (__sx_dig),a
        push    hl
        call    sx_accum                ; acc = acc*base + digit
        pop     hl
sx_loop_adv:
        inc     hl
        jr      sx_loop
sx_loop_end:
        ld      a,(__sx_any)
        or      a
        jr      z,sx_done               ; no digits: endptr = nptr
        ; endptr = p (HL)
        ld      a,(__sx_endp)
        ld      c,a
        ld      a,(__sx_endp + 1)
        ld      b,a
        or      c
        ret     z                       ; endptr == NULL
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        ret
sx_done:
        ; failure: *endptr = nptr (if endptr != 0)
        ld      a,(__sx_endp)
        ld      c,a
        ld      a,(__sx_endp + 1)
        ld      b,a
        or      c
        ret     z
        ld      a,(__sx_nptr)
        ld      (bc),a
        inc     bc
        ld      a,(__sx_nptr + 1)
        ld      (bc),a
        ret

        ; sx_digitval: A = char -> A = 0..35, or 0xFF if not a digit/letter
sx_digitval:
        cp      #0x30
        jr      c,sx_dv_bad
        cp      #0x3a                   ; '9'+1
        jr      c,sx_dv_dig
        cp      #0x41                   ; 'A'
        jr      c,sx_dv_bad
        cp      #0x5b                   ; 'Z'+1
        jr      c,sx_dv_up
        cp      #0x61                   ; 'a'
        jr      c,sx_dv_bad
        cp      #0x7b                   ; 'z'+1
        jr      c,sx_dv_lo
sx_dv_bad:
        ld      a,#0xff
        ret
sx_dv_dig:
        sub     #0x30
        ret
sx_dv_up:
        sub     #0x37                   ; 'A'(0x41) - 10
        ret
sx_dv_lo:
        sub     #0x57                   ; 'a'(0x61) - 10
        ret

        ; sx_accum: __sx_acc = __sx_acc * base + digit (digit in __sx_dig),
        ; setting __sx_ovf on 64-bit overflow.  clobbers AF,BC,DE,HL.
sx_accum:
        ; __sx_tmp = 0
        ld      hl,#__sx_tmp
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        ld      bc,#7
        ldir
        ; repeat 'base' times: tmp += acc
        ld      a,(__sx_base)
        ld      b,a
sx_mul:
        push    bc
        ld      hl,#__sx_tmp
        ld      de,#__sx_acc
        call    sx_add64                ; tmp += acc ; CF = carry out
        jr      nc,sx_mul_nc
        ld      a,#1
        ld      (__sx_ovf),a
sx_mul_nc:
        pop     bc
        djnz    sx_mul
        ; tmp += digit
        ld      hl,#__sx_tmp
        ld      a,(__sx_dig)
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
        ld      a,#1
        ld      (__sx_ovf),a
sx_acc_copy:
        ld      hl,#__sx_tmp
        ld      de,#__sx_acc
        ld      bc,#8
        ldir
        ret

        ; sx_add64: (HL)[8] += (DE)[8] ; returns CF = carry out of bit 63
sx_add64:
        or      a                       ; clear carry
        ld      b,#8
sx_add64_l:
        ld      a,(de)
        adc     a,(hl)
        ld      (hl),a
        inc     hl
        inc     de
        djnz    sx_add64_l
        ret

        ; __sx_negate: two's-complement negate __sx_acc[8] in place.
        ; clobbers AF, B, HL
__sx_negate::
        ld      hl,#__sx_acc
        ld      b,#8
sxn_cpl:
        ld      a,(hl)
        cpl
        ld      (hl),a
        inc     hl
        djnz    sxn_cpl
        ld      hl,#__sx_acc
        ld      b,#8
        scf
sxn_inc:
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        inc     hl
        djnz    sxn_inc
        ret
