        ; ieee16_common.s
        ;
        ; Internal IEEE-754 binary16 helpers shared by the public ieee16_*
        ; entry points. These are kept in a separate object so the public
        ; runtime surface can stay one-function-per-file for archive
        ; granularity, while still sharing the raw half classification and
        ; comparison logic.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ieee16_common
        .optsdcc -mz80 sdcccall(1)

        .globl  ___ieee16_classify_hl
        .globl  ___ieee16_cmp_hl_de
        .globl  ___ieee16_load_half_ptr_hl
        .globl  ___ieee16_store_half_ptr_de

        .area   _CODE

        ; inputs: HL = raw binary16
        ; outputs: DE = FP_NAN/FP_INFINITE/FP_ZERO/FP_SUBNORMAL/FP_NORMAL
___ieee16_classify_hl:
        ld      a,h
        and     #0x7c
        ld      b,a
        ld      a,h
        and     #0x03
        or      l
        ld      c,a
        ld      a,b
        cp      #0x7c
        jr      z,.class_inf_nan
        or      a
        jr      z,.class_zero_sub
        ld      de,#4
        ret
.class_inf_nan:
        ld      a,c
        or      a
        jr      z,.class_inf
        ld      de,#0
        ret
.class_inf:
        ld      de,#1
        ret
.class_zero_sub:
        ld      a,c
        or      a
        jr      z,.class_zero
        ld      de,#3
        ret
.class_zero:
        ld      de,#2
        ret

        ; Compare x=HL and y=DE as binary16 values.
        ; outputs: A = 0xff when x<y, 0 when x==y, 1 when x>y
        ; notes: +/-0 compare equal. NaNs are not specially ordered here.
___ieee16_cmp_hl_de:
        ld      a,h
        and     #0x7f
        or      l
        ld      b,a
        ld      a,d
        and     #0x7f
        or      e
        or      b
        jr      nz,.cmp_nonzero
        xor     a
        ret
.cmp_nonzero:
        bit     7,h
        jr      z,.cmp_x_pos
        bit     7,d
        jr      z,.cmp_lt
        ld      a,h
        cp      d
        jr      nz,.cmp_neg_hi
        ld      a,l
        cp      e
        jr      z,.cmp_eq
        jr      c,.cmp_gt
        jr      .cmp_lt
.cmp_neg_hi:
        jr      c,.cmp_gt
        jr      .cmp_lt
.cmp_x_pos:
        bit     7,d
        jr      nz,.cmp_gt
        ld      a,h
        cp      d
        jr      nz,.cmp_pos_hi
        ld      a,l
        cp      e
        jr      z,.cmp_eq
        jr      c,.cmp_lt
        jr      .cmp_gt
.cmp_pos_hi:
        jr      c,.cmp_lt
        jr      .cmp_gt
.cmp_eq:
        xor     a
        ret
.cmp_lt:
        ld      a,#0xff
        ret
.cmp_gt:
        ld      a,#0x01
        ret

        ; inputs: HL = pointer to binary16 object
        ; outputs: HL = raw binary16
___ieee16_load_half_ptr_hl:
        ld      e,(hl)
        inc     hl
        ld      h,(hl)
        ld      l,e
        ret

        ; inputs: HL = pointer to binary16 object, DE = raw binary16
___ieee16_store_half_ptr_de:
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret
