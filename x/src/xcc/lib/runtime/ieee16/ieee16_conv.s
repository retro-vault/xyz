        ; ieee16_conv.s
        ;
        ; IEEE-754 binary16 <-> binary32 conversion helpers for the xcc Z80
        ; runtime. These keep the public float32 libc surface reusable while
        ; allowing --float-format=ieee16 to use a real 16-bit IEEE float ABI.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ieee16_conv
        .optsdcc -mz80 sdcccall(1)

        .globl  ___fh2fs
        .globl  ___fs2fh

        .area   _CODE

        ; ___fh2fs
        ; inputs:  HL = IEEE-754 binary16 (H high byte, L low byte)
        ; outputs: HL:DE = IEEE-754 binary32 (HL high word, DE low word)
        ; clobbers: AF, BC, DE, HL
___fh2fs:
        ld      a,h
        and     #0x7c
        rrca
        rrca
        ld      b,a                     ; B = exp5

        ld      a,h
        and     #0x80
        ld      c,a                     ; C = sign bit

        ld      a,h
        and     #0x03
        ld      d,a                     ; D:E = mantissa10
        ld      e,l

        ld      a,b
        or      a
        jr      nz,.fh_have_exp

        ld      a,d
        or      e
        jr      nz,.fh_subnormal

        ; signed zero
        ld      h,c
        ld      l,#0x00
        ld      d,#0x00
        ld      e,#0x00
        ret

.fh_subnormal:
        ld      h,d
        ld      l,e
        ld      b,#113                  ; biased exp for 2^-14 in float32
.fh_subnormal_norm:
        bit     2,h                     ; hidden 1 reaches bit10 of the 16-bit mantissa
        jr      nz,.fh_subnormal_done
        add     hl,hl
        dec     b
        jr      .fh_subnormal_norm
.fh_subnormal_done:
        res     2,h
        jr      .fh_pack

.fh_have_exp:
        ld      a,b
        cp      #31
        jr      z,.fh_special

        ld      h,d
        ld      l,e
        ld      a,b
        add     a,#112                  ; rebias 5-bit half exponent to 8-bit float exponent
        ld      b,a
        jr      .fh_pack

.fh_special:
        ld      a,d
        or      e
        jr      z,.fh_inf

        ; quiet NaN
        ld      a,c
        or      #0x7f
        ld      h,a
        ld      l,#0xc0
        ld      d,#0x00
        ld      e,#0x00
        ret

.fh_inf:
        ld      a,c
        or      #0x7f
        ld      h,a
        ld      l,#0x80
        ld      d,#0x00
        ld      e,#0x00
        ret

.fh_pack:
        ; HL = mantissa10 fraction, B = biased float32 exponent, C = sign
        ld      a,h
        and     #0x03
        rlca
        rlca
        rlca
        rlca
        rlca
        ld      d,a                     ; mantissa bits 9..8 into byte2 bits 6..5

        ld      a,l
        srl     a
        srl     a
        srl     a
        or      d
        bit     0,b
        jr      z,.fh_store_byte2
        or      #0x80
.fh_store_byte2:
        ld      d,a                     ; save float byte2

        ld      a,l
        and     #0x07
        rlca
        rlca
        rlca
        rlca
        rlca
        ld      e,a                     ; save float byte1

        ld      a,b
        srl     a
        or      c
        ld      h,a                     ; float byte3
        ld      l,d                     ; float byte2
        ld      d,e                     ; float byte1
        ld      e,#0x00                 ; float byte0
        ret

        ; ___fs2fh
        ; inputs:  HL:DE = IEEE-754 binary32 (HL high word, DE low word)
        ; outputs: DE = IEEE-754 binary16 (D high byte, E low byte)
        ; clobbers: AF, BC, DE, HL, IX
___fs2fh:
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        push    de

        ; B = biased float32 exponent
        ld      a,7(ix)
        and     #0x7f
        rlca
        ld      b,a
        bit     7,6(ix)
        jr      z,.fs_exp_ready
        inc     b
.fs_exp_ready:

        ; C = sign bit for the binary16 result
        ld      a,7(ix)
        and     #0x80
        ld      c,a

        ld      a,b
        cp      #0xff
        jr      z,.fs_special
        or      a
        jp      z,.fs_zero              ; float32 zero and subnormals underflow to zero in binary16

        ld      a,b
        cp      #143                    ; half biased exponent 31 and above -> overflow
        jp      nc,.fs_inf
        cp      #113                    ; half biased exponent 1..30 -> normal
        jr      nc,.fs_normal
        cp      #102                    ; exp 102 may still round up to min subnormal
        jp      c,.fs_zero

        ; half subnormal: rounded(sig24 >> (126 - exp8))
        ld      a,#126
        sub     b
        ld      e,a                     ; shift count
        ld      a,6(ix)
        and     #0x7f
        or      #0x80
        ld      b,a
        ld      h,5(ix)
        ld      l,4(ix)
        ld      a,e
        call    .round_shr24

        ; rounded up into the minimum normal half?
        ld      a,h
        cp      #0x04
        jr      nz,.fs_sub_store
        ld      a,l
        or      a
        jr      nz,.fs_sub_store
        ld      a,c
        or      #0x04
        ld      d,a
        ld      e,#0x00
        jr      .fs_done

.fs_sub_store:
        ld      a,c
        or      h
        ld      d,a
        ld      e,l
        jr      .fs_done

.fs_normal:
        ld      a,b
        sub     #112
        ld      e,a                     ; E = half biased exponent 1..30
        ld      a,6(ix)
        and     #0x7f
        or      #0x80
        ld      b,a
        ld      h,5(ix)
        ld      l,4(ix)
        ld      a,#13
        call    .round_shr24

        ; carry from rounding overflowed 1.111... to 10.000...
        bit     3,h
        jr      z,.fs_pack_normal
        ld      hl,#0x0400
        inc     e
        ld      a,e
        cp      #31
        jr      nc,.fs_inf

.fs_pack_normal:
        ld      a,e
        rlca
        rlca
        ld      d,a                     ; exponent << 2
        ld      a,h
        and     #0x03
        or      d
        or      c
        ld      d,a
        ld      e,l
        jr      .fs_done

.fs_special:
        ld      a,6(ix)
        and     #0x7f
        or      5(ix)
        or      4(ix)
        jr      z,.fs_inf

        ; quiet NaN
        ld      a,c
        or      #0x7e
        ld      d,a
        ld      e,#0x00
        jr      .fs_done

.fs_inf:
        ld      a,c
        or      #0x7c
        ld      d,a
        ld      e,#0x00
        jr      .fs_done

.fs_zero:
        ld      d,c
        ld      e,#0x00

.fs_done:
        ld      sp,ix
        pop     ix
        ret

        ; round_shr24
        ; inputs:  B:HL = 24-bit unsigned value, A = shift count (1..24)
        ; outputs: B:HL = round-to-nearest-even(value >> shift)
        ; clobbers: AF, C, B, H, L
.round_shr24:
        ld      c,#0x00                 ; sticky bit accumulator
.round_loop:
        srl     b
        rr      h
        rr      l
        dec     a
        jr      z,.round_final
        jr      nc,.round_loop
        ld      c,#0x01
        jr      .round_loop

.round_final:
        jr      nc,.round_done
        bit     0,l
        jr      nz,.round_inc
        ld      a,c
        or      a
        jr      z,.round_done

.round_inc:
        inc     l
        ret     nz
        inc     h
        ret     nz
        inc     b

.round_done:
        ret
