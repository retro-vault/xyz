        ; float multiply (ieee-754 single) for the runtime
        ; result = a * b
        ; denormals treated as 0; NaN/Inf unsupported.
        ;
        ; ABI (sdcccall(1)):
        ; a in regs: HLDE  (H=a3, L=a2, D=a1, E=a0)
        ; b on stack: 4 bytes pushed by caller (low word first)
        ; result in HLDE
        ; callee cleans b from stack
        ;
        ; IEEE-754 single layout (big-endian byte order):
        ; byte3: S EEEEEEE    (H for a, ix+7 for b)
        ; byte2: E MMMMMMM    (L for a, ix+6 for b)
        ; byte1: MMMMMMMM     (D for a, ix+5 for b)
        ; byte0: MMMMMMMM     (E for a, ix+4 for b)
        ;

        .module fsmul_core
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___fsmul
        .globl  __fp_retpop4
        .globl  __fp_unpack_sign_exps
        .globl  __fp_unpack_mant24_ab
        .globl  __fp_pack_norm
        .globl  __fp_zero32

        ; ============================================================
        ; Frame layout:
        ;
        ; ix+7 : b3  (sign+exp high of b)
        ; ix+6 : b2  (exp low + mant high of b)
        ; ix+5 : b1
        ; ix+4 : b0
        ; ix+2,3: return address
        ; ix+0,1: saved ix
        ; ix-1 : H = a3        \  push hl
        ; ix-2 : L = a2        /
        ; ix-3 : D = a1        \  push de
        ; ix-4 : E = a0        /
        ; ix-5 : result sign
        ; ix-6 : result exponent
        ; ix-7  : mant_a[0]  (LSB)
        ; ix-8  : mant_a[1]
        ; ix-9  : mant_a[2]  (MSB, with implicit 1)
        ; ix-10 : mant_b[0]  (LSB)
        ; ix-11 : mant_b[1]
        ; ix-12 : mant_b[2]  (MSB, with implicit 1)
        ; ix-13 : prod[0]    (LSB)
        ; ix-14 : prod[1]
        ; ix-15 : prod[2]
        ; ix-16 : prod[3]
        ; ix-17 : prod[4]
        ; ix-18 : prod[5]    (MSB)
        ; ============================================================

        ; ___fsmul
        ; inputs: a in HLDE, b on caller stack (4 bytes)
        ; outputs: HLDE = IEEE-754 single product a * b
        ; clobbers: af, bc, de, hl, ix

___fsmul:
        push    ix
        ld      ix,#0
        add     ix,sp

        ; save operand a
        ; ix-1=H(a3), ix-2=L(a2)
        push    hl
        ; ix-3=D(a1), ix-4=E(a0)
        push    de

        ; allocate locals (14 bytes)
        ld      hl,#-14
        add     hl,sp
        ld      sp,hl

        ; ---- extract result sign and exponents ----
        call    __fp_unpack_sign_exps

        ; ---- check for zero exponent ----
        ld      a,c
        or      a
        jp      z,.ret_zero
        ld      a,b
        or      a
        jp      z,.ret_zero

        ; ---- result exponent: EA + EB - 127 ----
        ld      a,c
        add     a,b
        jr      c,.exp_carry
        cp      #127
        jp      c,.ret_zero
        sub     #127
        jr      .exp_store

.exp_carry:
        add     a,#129
        jp      c,.ret_inf

.exp_store:
        ld      -6(ix),a

        ; ---- build mantissas A/B (with implicit 1) ----
        call    __fp_unpack_mant24_ab

        ; ---- zero 48-bit product ----
        xor     a
        ld      -13(ix),a
        ld      -14(ix),a
        ld      -15(ix),a
        ld      -16(ix),a
        ld      -17(ix),a
        ld      -18(ix),a

        ; ---- 24x24 multiply: nine 8x8 partial products ----

        ; a[0] * b[0] -> prod[1:0]
        ld      l,-7(ix)
        ld      h,-10(ix)
        call    .mul8x8
        ld      -13(ix),l
        ld      -14(ix),h

        ; a[0] * b[1] -> prod[2:1]
        ld      l,-7(ix)
        ld      h,-11(ix)
        call    .mul8x8
        ld      a,-14(ix)
        add     a,l
        ld      -14(ix),a
        ld      a,-15(ix)
        adc     a,h
        ld      -15(ix),a
        jr      nc,.pp02
        inc     -16(ix)
.pp02:
        ; a[0] * b[2] -> prod[3:2]
        ld      l,-7(ix)
        ld      h,-12(ix)
        call    .mul8x8
        ld      a,-15(ix)
        add     a,l
        ld      -15(ix),a
        ld      a,-16(ix)
        adc     a,h
        ld      -16(ix),a
        jr      nc,.pp10
        inc     -17(ix)
.pp10:
        ; a[1] * b[0] -> prod[2:1]
        ld      l,-8(ix)
        ld      h,-10(ix)
        call    .mul8x8
        ld      a,-14(ix)
        add     a,l
        ld      -14(ix),a
        ld      a,-15(ix)
        adc     a,h
        ld      -15(ix),a
        jr      nc,.pp11
        inc     -16(ix)
        jr      nz,.pp11
        inc     -17(ix)
.pp11:
        ; a[1] * b[1] -> prod[3:2]
        ld      l,-8(ix)
        ld      h,-11(ix)
        call    .mul8x8
        ld      a,-15(ix)
        add     a,l
        ld      -15(ix),a
        ld      a,-16(ix)
        adc     a,h
        ld      -16(ix),a
        jr      nc,.pp12
        inc     -17(ix)
        jr      nz,.pp12
        inc     -18(ix)
.pp12:
        ; a[1] * b[2] -> prod[4:3]
        ld      l,-8(ix)
        ld      h,-12(ix)
        call    .mul8x8
        ld      a,-16(ix)
        add     a,l
        ld      -16(ix),a
        ld      a,-17(ix)
        adc     a,h
        ld      -17(ix),a
        jr      nc,.pp20
        inc     -18(ix)
.pp20:
        ; a[2] * b[0] -> prod[3:2]
        ld      l,-9(ix)
        ld      h,-10(ix)
        call    .mul8x8
        ld      a,-15(ix)
        add     a,l
        ld      -15(ix),a
        ld      a,-16(ix)
        adc     a,h
        ld      -16(ix),a
        jr      nc,.pp21
        inc     -17(ix)
        jr      nz,.pp21
        inc     -18(ix)
.pp21:
        ; a[2] * b[1] -> prod[4:3]
        ld      l,-9(ix)
        ld      h,-11(ix)
        call    .mul8x8
        ld      a,-16(ix)
        add     a,l
        ld      -16(ix),a
        ld      a,-17(ix)
        adc     a,h
        ld      -17(ix),a
        jr      nc,.pp22
        inc     -18(ix)
.pp22:
        ; a[2] * b[2] -> prod[5:4]
        ld      l,-9(ix)
        ld      h,-12(ix)
        call    .mul8x8
        ld      a,-17(ix)
        add     a,l
        ld      -17(ix),a
        ld      a,-18(ix)
        adc     a,h
        ld      -18(ix),a

        ; ---- normalize ----
        ld      b,-5(ix)
        ld      c,-6(ix)

        bit     7,-18(ix)
        jr      z,.no_shift

        ; bit47=1: shift right, exp++
        inc     c
        jr      z,.ret_inf

        ld      e,-16(ix)
        ld      d,-17(ix)
        ld      a,-18(ix)
        and     #0x7F
        ld      l,a
        jr      .pack

.no_shift:
        ; bit46=1: shift prod[5:2] left by 1
        ld      a,-15(ix)
        add     a,a
        ld      a,-16(ix)
        rla
        ld      e,a
        ld      a,-17(ix)
        rla
        ld      d,a
        ld      a,-18(ix)
        rla
        and     #0x7F
        ld      l,a

.pack:
        call    __fp_pack_norm

        jr      .cleanup

.ret_zero:
        call    __fp_zero32
        jr      .cleanup

.ret_inf:
        ld      a,-5(ix)
        or      #0x7F
        ld      h,a
        ld      l,#0x80
        ld      d,#0
        ld      e,#0

.cleanup:
        ld      sp,ix
        pop     ix
        jp      __fp_retpop4

        ; ============================================================
        ; .mul8x8: unsigned 8x8 -> 16-bit multiply
        ; Input:  L = multiplicand, H = multiplier
        ; Output: HL = product
        ; clobbers: A, B, DE
        ; ============================================================
.mul8x8:
        ld      d,#0
        ld      e,l
        ld      l,#0
        ld      a,h
        ld      h,#0
        ld      b,#8
.mul8_loop:
        rra
        jr      nc,.mul8_skip
        add     hl,de
.mul8_skip:
        sla     e
        rl      d
        djnz    .mul8_loop
        ret
