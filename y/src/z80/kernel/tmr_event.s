        ; Event timers never execute application code in interrupt context.
        .module tmr_event
        .optsdcc -mz80 sdcccall(1)
        .globl _tmr_event, _tmr_install, __current_process
        .globl __critical_call
        .area _ROM_AUX
        ; HL=live event, DE=period (fires every period+1 frames).
        ; Bank FE tags an event pointer in the existing timer handler slot.
_tmr_event::
        ld a,h
        or l
        jr nz,.install
        ex de,hl                        ; a null event cannot be signaled
        ret
.install:
        call __critical_call
        call __current_process
        push bc
        call _tmr_install
        ld a,d
        or e
        jr z,.done
        ld hl,#11
        add hl,de
        ld (hl),#0xfe
.done:
        ret
