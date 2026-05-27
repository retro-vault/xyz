        ; float equal (ieee-754 single) for the runtime
        ; returns 1 if a==b else 0
        ; denormals treated as 0; NaN/Inf unsupported.
        ;
        ; ABI (observed):
        ;   a in regs: HL:DE
        ;   b on stack: ret, b.low, b.high
        ;   result in A (0/1)
        ;
        ; clobbers: af, bc, de, hl, ix
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fseq
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___fseq
        .globl  ___fscmp

        ; ___fseq
        ; inputs:  a in HL:DE, b on stack (2 words after return address)
        ; outputs: A = 1 if a == b else 0
        ; clobbers: af, bc, de, hl, ix
___fseq:
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

        ; Check if DE == 0
        ld      a,d
        or      e
        jr      nz, .ret_false

        ; result was 0, so a == b
        ld      a,#1
        ret

.ret_false:
        xor     a                       ; A = 0
        ret
