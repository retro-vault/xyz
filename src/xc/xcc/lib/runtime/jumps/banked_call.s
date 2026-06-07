        ; Shared banked call and return helpers
        ; supports --model-large compilation with banked memory.
        ;
        ; when the compiler emits a banked call, the call instruction
        ; is followed by a 4-byte inline descriptor:
        ;   [+0..+1] target function address (little-endian)
        ;   [+2..+3] target bank number     (little-endian)
        ;
        ; NOTE: bank switching is platform-specific. the two hook
        ; points (.switch_bank / .restore_bank) must be adapted to
        ; your hardware. as written they are no-ops (flat memory).
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module banked_call
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___sdcc_bcall
        .globl  ___sdcc_bcall_ehl
        .globl  __sdcc_bcall
        .globl  __sdcc_banked_call
        .globl  __sdcc_banked_ret

___sdcc_bcall:
        ; __sdcc_bcall
        ; compatibility alias used by sdcc runtime naming.
__sdcc_bcall:

        ; __sdcc_banked_call
        ; inputs:  inline 4-byte descriptor after call (addr + bank)
        ; outputs: n/a (calls target, then returns past descriptor)
        ; clobbers: af (others depend on target)
__sdcc_banked_call:
        pop     hl

        ; read target address
        ld      e, (hl)
        inc     hl
        ld      d, (hl)                 ; de = target function address
        inc     hl

        ; read target bank
        ld      c, (hl)
        inc     hl
        ld      b, (hl)                 ; bc = target bank number
        inc     hl

        ; hl now points past the descriptor (our true return address)
        push    hl

        ; ---- platform hook: switch to bank bc ----
        ; replace the nops below with your bank switching logic
        ; e.g. ld a, c / out (BANK_PORT), a
        nop
        nop
        ; ---- end platform hook ----

        ; call the target function
        push    de
        pop     hl                      ; hl = target address
        jp      (hl)

        ; ___sdcc_bcall_ehl
        ; banked-call helper variant used by some sdcc large-model
        ; paths:
        ;   e = bank id, hl = target address.
        ; with flat-memory default hooks this degrades to an indirect
        ; jump.
___sdcc_bcall_ehl:
        jp      (hl)

        ; __sdcc_banked_ret
        ; inputs:  n/a
        ; outputs: n/a (returns to caller with bank restored)
        ; clobbers: af
__sdcc_banked_ret:

        ; ---- platform hook: restore previous bank ----
        ; replace the nops below with your bank restore logic
        nop
        nop
        ; ---- end platform hook ----

        ret
