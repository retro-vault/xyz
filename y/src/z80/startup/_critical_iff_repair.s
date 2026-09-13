        ; Repair the NMOS LD A,I parity race at critical-section entry.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih

        .module _critical_iff_repair
        .optsdcc -mz80 sdcccall(1)
        .globl  __critical_iff_repair
        .globl  __critical_iff_sampled
        .area   _CODE

        ; Called by IM2 after PUSH AF / PUSH HL, before switching threads.
        ; Stack: helper return, saved HL, saved AF, interrupted PC.
        ; IRQ acceptance proves IFF was enabled at this precise sample.
        ; Set only saved P/V; preserve every other interrupted register.
        ; inputs: scheduler stack; clobbers: AF, HL (already saved)
__critical_iff_repair::
        push    de
        ld      hl,#8
        add     hl,sp
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      hl,#__critical_iff_sampled
        or      a
        sbc     hl,de
        pop     de
        ret     nz
        ld      hl,#4
        add     hl,sp
        set     2,(hl)
        ret
