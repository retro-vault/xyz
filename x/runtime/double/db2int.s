        ; double to integer conversions (truncate toward zero)
        ; all take the double in DE:HL:DE':HL'
        ;
        ; signed:   db2sint (DE), db2slong (DE:HL), [db2sll core elsewhere]
        ; unsigned: db2uint (DE), db2ulong (DE:HL), db2ull (regs) — neg → 0
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module db2int
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___db2sint
        .globl  ___db2uint
        .globl  ___db2slong
        .globl  ___db2ulong
        .globl  ___db2ull
        .globl  ___db2sll
        .globl  __db_zero

        ; ___db2sint — double → int16 in DE, saturating to [-32768, 32767]
___db2sint:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, h
        ld      c, l
        ld      hl, #-8
        add     hl, sp
        ld      sp, hl
        ld      h, b
        ld      l, c
        ; store double bytes a0..a7
        ld      -8(ix), e
        ld      -7(ix), d
        ld      -6(ix), l
        ld      -5(ix), h
        exx
        ld      -4(ix), e
        ld      -3(ix), d
        ld      -2(ix), l
        ld      -1(ix), h
        exx
        ; biased exp = ((a7&0x7F)<<4)|(a6>>4)
        ld      a, -1(ix)
        and     #0x7F
        ld      h, #0
        ld      l, a
        add     hl, hl
        add     hl, hl
        add     hl, hl
        add     hl, hl
        ld      a, -2(ix)
        rlca
        rlca
        rlca
        rlca
        and     #0x0F
        or      l
        ld      l, a            ; HL = biased exp
        ; |x| >= 32768 iff biased exp >= 1038 (0x40E)
        ld      de, #0x040E
        or      a
        sbc     hl, de
        jp      c, .ds_inrange
        ; saturate by sign
        bit     7, -1(ix)
        ld      de, #0x7FFF
        jp      z, .ds_ret
        ld      de, #0x8000
.ds_ret:
        ld      sp, ix
        pop     ix
        ret
.ds_inrange:
        ; reload double into registers and convert
        ld      e, -8(ix)
        ld      d, -7(ix)
        ld      l, -6(ix)
        ld      h, -5(ix)
        exx
        ld      e, -4(ix)
        ld      d, -3(ix)
        ld      l, -2(ix)
        ld      h, -1(ix)
        exx
        ld      sp, ix
        pop     ix
        jp      ___db2sll       ; DE = low16 after conversion

        ; ___db2slong — double → int32 in DE:HL (low 32)
___db2slong:
        jp      ___db2sll       ; DE:HL = low32 after conversion

        ; ___db2uint — double → uint16 in DE; negative → 0
___db2uint:
        bit     7, h            ; H is byte3? no — sign is in H' bit7
        ; check sign: need H' bit7
        exx
        bit     7, h
        exx
        jp      nz, .u_zero
        jp      ___db2sll
.u_zero:
        jp      __db_zero

        ; ___db2ulong — double → uint32 in DE:HL; negative → 0
___db2ulong:
        exx
        bit     7, h
        exx
        jp      nz, .ul_zero
        jp      ___db2sll
.ul_zero:
        jp      __db_zero

        ; ___db2ull — double → uint64 in regs; negative → 0
___db2ull:
        exx
        bit     7, h
        exx
        jp      nz, .ull_zero
        jp      ___db2sll
.ull_zero:
        jp      __db_zero
