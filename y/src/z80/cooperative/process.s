        ;; Cooperative ABI-1 processes. UI work runs on the resident event
        ;; stack; process stacks yield at wait_event, never inside filesystem calls.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_process
        .optsdcc -mz80 sdcccall(1)
        .include "native.inc"
        .globl __native_init, __native_done, __native_poll, __native_processes
        .globl __native_current, __native_yield, __native_exit, __native_start
        .globl __native_query, __native_far28, __native_far20, __native_far30
        .globl __native_events, __native_timers, __native_service_reap
        .globl __native_browser_poll
        .globl __native_defer, __native_tick, __native_last_tick
        .globl __native_event_create, __native_event_destroy, __native_event_set
        .globl __native_event_wait, __native_timer_create, __native_timer_destroy
        .globl __native_ted_poll
        .globl __native_sys_snapshot
        .globl __native_fs_reap, __native_memory_reap, __native_library_gc
        .globl _free, __yos_clock, ___sdcc_call_bc
        .globl __partner_bank_select, __partner_image_free, __far_putb
        .area _BSS
__native_processes:: .ds P_BYTES*P_COUNT
__native_current:: .ds 2
.host_sp: .ds 2
.area _KERNEL_BSS
.saved_vectors: .ds 27
__native_events:: .ds 64             ; 16 {active,state,owner word}
__native_timers:: .ds 128            ; 16 {active,event,period,next,owner index}
__native_last_tick:: .ds 2
.area _BSS
.jobs: .ds 32                       ; eight {job,process} records
.job_head: .ds 1
.job_tail: .ds 1
.job_count: .ds 1
        .area _CODE
__native_init::
        ld hl,#0x18
        ld de,#.saved_vectors
        ld bc,#27
        ldir
        ld a,#0xc3
        ld (0x18),a
        ld (0x20),a
        ld (0x28),a
        ld (0x30),a
        ld hl,#__native_query
        ld (0x19),hl
        ld hl,#__native_far20
        ld (0x21),hl
        ld hl,#__native_far28
        ld (0x29),hl
        ld hl,#__native_far30
        ld (0x31),hl
        ld hl,#0x18
        ld b,#27
.vectors_bank2:
        ld a,(hl)
        ld c,#2
        call __far_putb
        inc hl
        djnz .vectors_bank2
        call __yos_clock
        ld (__native_last_tick),de
        ret
__native_done::
        ;; All windows have already received DESTROY while their code and RST
        ;; gates were live. Cancel queued callbacks before reclaiming images.
        xor a
        ld (.job_count),a
        ld (.job_head),a
        ld (.job_tail),a
        push ix
        ld ix,#__native_processes
        ld b,#P_COUNT
.stop:
        ld a,P_STATE(ix)
        or a
        jr z,.stop_next
        ld P_STATE(ix),#2
.stop_next:
        ld de,#P_BYTES
        add ix,de
        djnz .stop
        pop ix
        call __native_poll
        ld hl,#.saved_vectors
        ld de,#0x18
        ld bc,#27
        ldir
        ret
__native_start::
        ld hl,(__native_current)
        ld de,#P_ENTRY
        add hl,de
        ld c,(hl)
        inc hl
        ld b,(hl)
        call ___sdcc_call_bc
__native_exit::
        ld hl,(__native_current)
        ld de,#P_STATE
        add hl,de
        ld (hl),#2
        call __native_yield
        jr __native_exit
__native_yield::
        push af
        push bc
        push de
        push hl
        push ix
        push iy
        ld hl,#0
        add hl,sp
        ex de,hl
        ld hl,(__native_current)
        ld bc,#P_SP
        add hl,bc
        ld (hl),e
        inc hl
        ld (hl),d
        push hl
        ld hl,(__native_current)
        ld bc,#P_RESUME
        add hl,bc
        ld a,(0xc002)
        ld (hl),a
        pop hl
        ld a,#1
        call __partner_bank_select
        ld sp,(.host_sp)
        ret
.resume:
        ld (__native_current),hl
        ld (.host_sp),sp
        push hl
        ld de,#P_RESUME
        add hl,de
        ld a,(hl)
        call __partner_bank_select
        pop hl
        ld de,#P_SP
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        ex de,hl
        ld sp,hl
        pop iy
        pop ix
        pop hl
        pop de
        pop bc
        pop af
        ret
__native_poll::
        push ix
        push iy
        call __native_tick
        ld ix,#__native_processes
        ld b,#P_COUNT
.next:
        push bc
        ld a,P_STATE(ix)
        cp #1
        jr nz,.reap
        push ix
        push ix
        pop hl
        call .resume
        pop ix
.reap:
        ld a,P_STATE(ix)
        cp #2
        jr nz,.advance
        call __native_fs_reap
        call __native_service_reap
        call __native_memory_reap
        ld l,P_IMAGE(ix)
        ld h,P_IMAGE+1(ix)
        ld a,P_BANK(ix)
        call __partner_image_free
        ld l,P_STACK(ix)
        ld h,P_STACK+1(ix)
        call _free
        ld l,P_ARGS(ix)
        ld h,P_ARGS+1(ix)
        call _free
        ;; Release this process's event slots; timers referencing them vanish.
        ld hl,#__native_events
        ld b,#16
.reap_event:
        inc hl
        inc hl
        ld e,(hl)
        inc hl
        ld d,(hl)
        dec hl
        dec hl
        dec hl
        push ix
        pop bc
        ld a,e
        cp c
        jr nz,.reap_next
        ld a,d
        cp b
        jr nz,.reap_next
        push hl
        call __native_event_destroy
        pop hl
.reap_next:
        ld de,#4
        add hl,de
        ;; End by address, since BC is the owner comparison scratch.
        ld de,#__native_events+64
        push hl
        or a
        sbc hl,de
        pop hl
        jr nz,.reap_event
        ld P_STATE(ix),#0
.advance:
        ld de,#P_BYTES
        add ix,de
        pop bc
        dec b
        jp nz,.next
        ld hl,#0
        ld (__native_current),hl
        call .poll_job
        call __native_ted_poll
        call __native_library_gc
        pop iy
        pop ix
        ret
        ;; HL={callback,completion}: enqueue without touching caller storage
        ;; after its lifetime. The completion byte is set after UI work returns.
__native_defer::
        ld a,(.job_count)
        cp #8
        jr nc,.job_full
        push hl
        inc hl
        inc hl
        ld (hl),#0
        ld a,(.job_tail)
        ld l,a
        ld h,#0
        ld de,#.jobs
        add hl,de
        pop de
        ld (hl),e
        inc hl
        ld (hl),d
        inc hl
        ld de,(__native_current)
        ld (hl),e
        inc hl
        ld (hl),d
        ld a,(.job_tail)
        add a,#4
        and #31
        ld (.job_tail),a
        ld hl,#.job_count
        inc (hl)
        xor a
        ret
.job_full:
        ld a,#255
        ret
.poll_job:
        ld a,(.job_count)
        or a
        ret z
        ld a,(.job_head)
        ld l,a
        ld h,#0
        ld de,#.jobs
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        push de
        inc hl
        ld e,(hl)
        inc hl
        ld d,(hl)
        ld (__native_current),de
        ex de,hl
        ld de,#P_BANK
        add hl,de
        ld a,(hl)
        inc a                          ; YOS bank -> physical bank
        call __partner_bank_select
        ld a,(.job_head)
        add a,#4
        and #31
        ld (.job_head),a
        ld hl,#.job_count
        dec (hl)
        pop hl
        push hl
        ld c,(hl)
        inc hl
        ld b,(hl)
        call ___sdcc_call_bc
        pop hl
        inc hl
        inc hl
        ld (hl),#1
        ld a,#1
        call __partner_bank_select
        ld hl,#0
        ld (__native_current),hl
        ret
__native_event_create::
        ld hl,#__native_events
        ld b,#16
.event_find:
        ld a,(hl)
        or a
        jr z,.event_free
        ld de,#4
        add hl,de
        djnz .event_find
        ld de,#0
        ret
.event_free:
        ld (hl),#1
        push hl
        inc hl
        ld (hl),#0
        inc hl
        ld de,(__native_current)
        ld (hl),e
        inc hl
        ld (hl),d
        pop de
        ret
__native_event_destroy::
        ld (hl),#0
        push ix
        ld ix,#__native_timers
        ld b,#16
.event_timer:
        ld a,1(ix)
        cp l
        jr nz,.event_timer_next
        ld a,2(ix)
        cp h
        jr nz,.event_timer_next
        ld 0(ix),#0
.event_timer_next:
        ld de,#8
        add ix,de
        djnz .event_timer
        pop ix
        ret
__native_event_set::
        pop bc
        dec sp                         ; only the discarded flags byte is below SP
        pop af                         ; A=state, without reading caller storage
        inc hl
        ld (hl),a
        dec hl
        ex de,hl
        push bc
        ret
__native_event_wait::
        push hl
.event_wait:
        pop hl
        push hl
        inc hl
        ld a,(hl)
        or a
        jr nz,.event_ready
        call __native_yield
        jr .event_wait
.event_ready:
        ld (hl),#0
        pop hl
        ret
        ;; HL=event, DE=period; a zero period means the next 50 Hz tick.
__native_timer_create::
        push ix
        push hl
        push de
        ld ix,#__native_timers
        ld b,#16
.timer_find:
        ld a,0(ix)
        or a
        jr z,.timer_found
        ld de,#8
        add ix,de
        djnz .timer_find
        pop de
        pop hl
        pop ix
        ld de,#0
        ret
.timer_found:
        pop de
        inc de
        ld a,d
        or e
        jr nz,.period_ready
        inc de
.period_ready:
        ld 3(ix),e
        ld 4(ix),d
        ld hl,(__native_last_tick)
        add hl,de
        ld 5(ix),l
        ld 6(ix),h
        pop hl
        ld 1(ix),l
        ld 2(ix),h
        ld 0(ix),#1
        push ix
        pop de
        pop ix
        ret
__native_timer_destroy::
        ld (hl),#0
        ret
__native_tick::
        call __yos_clock
        ld hl,(__native_last_tick)
        or a
        sbc hl,de
        ret z
        ld (__native_last_tick),de
        call __native_sys_snapshot
        call __native_browser_poll
        push ix
        ld ix,#__native_timers
        ld b,#16
.tick_next:
        push bc
        ld a,0(ix)
        or a
        jr z,.tick_skip
        ld e,5(ix)
        ld d,6(ix)
        ld hl,(__native_last_tick)
        or a
        sbc hl,de
        bit 7,h
        jr nz,.tick_skip
        ld l,1(ix)
        ld h,2(ix)
        inc hl
        ld (hl),#1
        ld e,3(ix)
        ld d,4(ix)
        ld hl,(__native_last_tick)
        add hl,de
        ld 5(ix),l
        ld 6(ix),h
.tick_skip:
        ld de,#8
        add ix,de
        pop bc
        djnz .tick_next
        pop ix
        ret
