        ; float less-than (ieee-754 single) for the runtime
        ; returns 1 if a<b else 0
        ; denormals treated as 0; NaN/Inf unsupported.
        ;
        ; ABI (observed):
        ;   a in regs: HL:DE (H=a3, L=a2, D=a1, E=a0)
        ;   b on stack: ret, b.low, b.high   (caller pushes b.low then
        ; b.high)
        ;   result in A (0/1)
        ;
        ; clobbers: af, bc, de, hl, ix
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fslt
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___fslt
        .globl  ___fscmp

        ; ___fslt
        ; inputs:  a in HL:DE, b on stack (2 words after return address)
        ; outputs: A = 1 if a < b else 0
        ; clobbers: af, bc, de, hl, ix
___fslt:
        ; Stack on entry: ret_to_caller, b.low, b.high
        ; a is in HL:DE (must preserve this!)

        exx
        pop     hl                      ; HL' = return address
        exx

        ; Now stack is: b.low, b.high (correct for fscmp)
        call    ___fscmp

        ; fscmp has cleaned stack and returned result in DE
        exx
        push    hl                      ; restore return address
        exx

        ; Check if DE == -1
        ld      a,d
        inc     a
        jr      nz, .ret_false
        ld      a,e
        inc     a
        jr      nz, .ret_false

        ld      a,#1
        ret

.ret_false:
        xor     a
        ret
