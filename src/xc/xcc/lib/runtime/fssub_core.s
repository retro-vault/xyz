        ; float sub (ieee-754 single) for the runtime
        ;
        ; computes a - b by flipping b's sign bit and tail-calling
        ; ___fsadd.
        ;
        ; ABI (same as ___fsadd):
        ; a in regs:  dehl = a0,a1,a2,a3 (little endian bytes)
        ; b on stack: push hl (b2,b3), push bc (b0,b1), call ___fssub
        ; callee cleanup is performed by ___fsadd (tail call).
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih
        ;

        .module fssub_core
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___fssub
        .globl  ___fsadd

        ; ___fssub
        ; inputs: a in DEHL, b on caller stack (4 bytes)
        ; outputs: DEHL = IEEE-754 single result a - b
        ; clobbers: af, bc, de, hl, ix

___fssub::
        ; preserve a2/a3 (HL)
        push    hl

        ; after push hl, SP moved by -2
        ; original b3 at (SP_old + 5) is now at (SP_new + 7)
        ld      hl,#7
        add     hl,sp
        ld      a,(hl)
        xor     #0x80
        ld      (hl),a

        ; restore a2/a3
        pop     hl
        jp      ___fsadd
