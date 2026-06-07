        ; ieee-754 single add for the runtime
        ;
        ; ABI (sdcccall(1), public caller-clean entry):
        ; a in regs:  dehl = a0,a1,a2,a3 (little endian bytes)
        ; e=a0 d=a1 l=a2 h=a3
        ; b on stack: push hl (b2,b3), push bc (b0,b1), call __fsadd
        ; caller cleans the 4-byte stack operand after return.
        ;
        ; return:
        ; dehl packed float (same byte order)
        ;
        ; behaviour:
        ; - denormals (exp==0) flushed to 0
        ; - no NaN/Inf handling
        ; - truncation (no rounding)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; (c) 2025 tomaz stih

        .module fsadd
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fsadd
        .globl  ___fsadd
        .globl  __fp_pack_norm
        .globl  __fp_zero32

        ; __fsadd / ___fsadd
        ; inputs: a in DEHL, b on caller stack (4 bytes)
        ; outputs: DEHL = IEEE-754 single sum
        ; clobbers: af, bc, de, hl, ix

__fsadd:
___fsadd::
        push    ix
        ld      ix,#0
        add     ix,sp

        ; preserve incoming a (dehl)
        push    de
        push    hl

        ; reserve locals
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl

        ; copy preserved a into locals
        ld      a,-2(ix)
        ; a0
        ld      -12(ix),a
        ld      a,-1(ix)
        ; a1
        ld      -11(ix),a
        ld      a,-4(ix)
        ; a2
        ld      -10(ix),a
        ld      a,-3(ix)
        ; a3
        ld      -9(ix),a

        ; load b from caller stack
        ld      a,4(ix)
        ; b0
        ld      -8(ix),a
        ld      a,5(ix)
        ; b1
        ld      -7(ix),a
        ld      a,6(ix)
        ; b2
        ld      -6(ix),a
        ld      a,7(ix)
        ; b3
        ld      -5(ix),a

        ; exponent extraction
        ; ea = ((a3&0x7f)<<1) | (a2>>7)
        ; eb = ((b3&0x7f)<<1) | (b2>>7)

        ; ea -> B
        ld      a,-9(ix)
        and     #0x7f
        rlca
        ld      b,a
        bit     7,-10(ix)
        jr      z,.ea_ok
        set     0,b
.ea_ok:
        ; eb -> C
        ld      a,-5(ix)
        and     #0x7f
        rlca
        ld      c,a
        bit     7,-6(ix)
        jr      z,.eb_ok
        set     0,c
.eb_ok:

        ; flush denormals
        ld      a,b
        or      a
        jr      nz,.ea_nz
        ; return b
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        jp      .ret_cleanup

.ea_nz:
        ld      a,c
        or      a
        jr      nz,.both_nz
        ; return a
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        jp      .ret_cleanup

.both_nz:
        ; signs
        ld      a,-9(ix)
        and     #0x80
        ; sa
        ld      -4(ix),a
        ld      a,-5(ix)
        and     #0x80
        ; sb
        ld      -3(ix),a

        ; choose X
        ld      a,b
        cp      c
        jr      z,.exp_eq
        jp      c,.x_is_b
        jr      .x_is_a

.exp_eq:
        ld      a,-10(ix)
        and     #0x7f
        or      #0x80
        ld      d,a
        ld      a,-6(ix)
        and     #0x7f
        or      #0x80
        cp      d
        jr      c,.x_is_a
        jr      nz,.x_is_b
        ld      a,-11(ix)
        cp      -7(ix)
        jr      c,.x_is_b
        jr      nz,.x_is_a
        ld      a,-12(ix)
        cp      -8(ix)
        jr      c,.x_is_b
        jr      nz,.x_is_a

        ; exact cancel
        ld      a,-4(ix)
        xor     -3(ix)
        jr      z,.x_is_a
        call    __fp_zero32
        jp      .ret_cleanup

.x_is_a:
        ld      a,b
        sub     c
        ld      -1(ix),a
        ld      -2(ix),b

        ; mant X
        ld      a,-10(ix)
        and     #0x7f
        or      #0x80
        ld      c,a
        ld      b,-11(ix)
        ld      l,-12(ix)

        ; mant Y
        ld      a,-6(ix)
        and     #0x7f
        or      #0x80
        ld      h,a
        ld      d,-7(ix)
        ld      e,-8(ix)
        jr      .align_y

.x_is_b:
        ld      a,c
        sub     b
        ld      -1(ix),a
        ld      -2(ix),c

        ; swap signs
        ld      a,-3(ix)
        ld      -4(ix),a
        ld      a,-9(ix)
        and     #0x80
        ld      -3(ix),a

        ; mant X
        ld      a,-6(ix)
        and     #0x7f
        or      #0x80
        ld      c,a
        ld      b,-7(ix)
        ld      l,-8(ix)

        ; mant Y
        ld      a,-10(ix)
        and     #0x7f
        or      #0x80
        ld      h,a
        ld      d,-11(ix)
        ld      e,-12(ix)

.align_y:
        ld      a,-1(ix)
        cp      #31
        jr      c,.sh_ok
        xor     a
        ld      h,a
        ld      d,a
        ld      e,a
        jr      .addsub

.sh_ok:
        or      a
        jr      z,.addsub
.sh_loop:
        srl     h
        rr      d
        rr      e
        dec     a
        jr      nz,.sh_loop

.addsub:
        ld      a,-4(ix)
        xor     -3(ix)
        jr      z,.do_add

        ; subtract
        ld      a,l
        sub     e
        ld      l,a
        ld      a,b
        sbc     a,d
        ld      b,a
        ld      a,c
        sbc     a,h
        ld      c,a

        ld      a,c
        or      b
        or      l
        jr      nz,.sub_norm
        call    __fp_zero32
        jr      .ret_cleanup

.sub_norm:
        ld      a,-2(ix)
.sub_loop:
        bit     7,c
        jr      nz,.sub_pack
        sla     l
        rl      b
        rl      c
        dec     a
        jr      nz,.sub_loop
        call    __fp_zero32
        jr      .ret_cleanup

.sub_pack:
        ld      -2(ix),a
        jr      .pack

.do_add:
        ld      a,l
        add     a,e
        ld      l,a
        ld      a,b
        adc     a,d
        ld      b,a
        ld      a,c
        adc     a,h
        ld      c,a
        jr      nc,.pack
        srl     c
        rr      b
        rr      l
        ld      a,-2(ix)
        inc     a
        ld      -2(ix),a

.pack:
        ld      e,l
        ld      d,b
        ld      a,c
        and     #0x7f
        ld      l,a
        ld      a,-2(ix)
        and     #0x01
        jr      z,.p2_ok
        set     7,l
.p2_ok:
        ld      b,-4(ix)
        ld      c,-2(ix)
        call    __fp_pack_norm

.ret_cleanup:
        ld      sp,ix
        pop     ix
        ret
