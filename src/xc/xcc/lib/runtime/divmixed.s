        ; signed/unsigned mixed division helpers (8-bit × 8-bit)
        ; handles a/signed dividend with u/signed divisor using 16-bit
        ; core
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2010-2021 philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module divmixed
        .optsdcc -mz80 sdcccall(1)

        .globl  __divsuchar_rrx_s
        .globl  __divsuchar_rrf_s
        .globl  __divsuchar             ; export symbols
        .globl  __divuschar_rrx_s
        .globl  __divuschar_rrf_s
        .globl  __divuschar
        .globl  __div16
        .globl  __div_signexte

        ; __divsuchar
        ; inputs:  a = signed dividend (8-bit), l = unsigned divisor
        ; (8-bit)
        ; outputs: de = quotient (16-bit), hl = remainder (16-bit)
        ; clobbers: a, d, e, h, l, f; tail-jumps to __div_signexte
        ; notes: build hl from dividend (h<-0, l<-a), e<-divisor,
        ;        then use mixed signed/unsigned divide core
__divsuchar_rrx_s::
__divsuchar_rrf_s::
__divsuchar:
        ld      e, l
        ld      l, a                    ; l = dividend low
        ld      h, #0
        jp      __div_signexte

        ; __divuschar
        ; inputs:  a = unsigned dividend (8-bit), l = signed divisor
        ; (8-bit)
        ; outputs: de = quotient (16-bit), hl = remainder (16-bit)
        ; clobbers: a, d, e, h, l, f; tail-jumps to __div16
        ; notes: e<-divisor, d<-0; sign-extend dividend into h, then
        ;        use signed 16-bit divide core
__divuschar_rrx_s::
__divuschar_rrf_s::
__divuschar:
        ld      e, l                    ; e = divisor (signed)
        ld      d, #0
        ld      l, a                    ; l = dividend low

        rlca
        sbc     a, a
        ld      h, a                    ; h = sign(dividend)

        jp      __div16                 ; signed 16-bit divide
