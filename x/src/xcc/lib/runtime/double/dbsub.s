        ; ieee-754 double subtract for the runtime
        ; computes a - b by flipping b's sign bit and tail-calling __dbadd
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbsub
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __dbsub
        .globl  ___dbsub
        .globl  __dbadd

        ; __dbsub / ___dbsub
        ; inputs:  a in DE:HL:DE':HL', b on stack (8 bytes at ix+4..ix+11)
        ; outputs: DE:HL:DE':HL' = a - b
        ; clobbers: af, bc, de, hl, ix, de', hl'

__dbsub:
___dbsub::
        ; After call, stack is: [ret_addr] [b0..b7]
        ; b7 = byte at sp+9 from top (2 bytes ret + 7 bytes b0..b6 before b7)
        ; We need to flip bit7 of b7 which is at sp+9 from entry SP.
        ; Use HL as scratch: HL' holds part of a, so save/restore via alt bank.
        ;
        ; SP on entry: [ret_addr_lo, ret_addr_hi, b0, b1, b2, b3, b4, b5, b6, b7, ...]
        ; b7 offset from SP = 2+7 = 9

        ; Save HL into BC (main bank scratch)
        ld      b, h
        ld      c, l

        ; Compute address of b7 on stack
        ld      hl, #9
        add     hl, sp
        ld      a, (hl)
        xor     #0x80
        ld      (hl), a

        ; Restore HL
        ld      h, b
        ld      l, c

        jp      __dbadd
