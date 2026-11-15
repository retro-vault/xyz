        ; Execute firmware commands without exposing banked text or callbacks
        ; while esxDOS owns the window. Retain its newest 1024 print bytes, as
        ; the Terminal does, then deliver them with the caller bank restored.
        ; HL=command, return native AF/CF. Preserve IX/IY. Fixed temporary
        ; output is owned by this call and always released. Serialized by the
        ; filesystem critical section; print sinks must not execute commands.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        ;
        .module divmmc_exec
        .optsdcc -mz80 sdcccall(1)
        .globl __divmmc_exec, __zx_esx_m_execcmd, __esx_print_hook
        .globl __os_malloc, __os_free, __divmmc_firmware, __divmmc_resume
        .globl _enter_critical_section, _leave_critical_section
        .area _DIVMMC_STATE
.command: .ds 2
.hook: .ds 2
.buffer: .ds 2
.cursor: .ds 2
.end: .ds 2
.count: .ds 2
.drive: .ds 1
        .globl __divmmc_command_return, __divmmc_replay, __divmmc_done
        .area _CODE
__divmmc_exec::
        call _enter_critical_section
        ld a,h
        or l
        jr nz,.command_ready
        ; Private firmware query: a NULL command returns M_GETSETDRV in A.
        call .drive_call
        jp _leave_critical_section
.command_ready:
        ld (.command),hl
        ld hl,(__esx_print_hook)
        ld (.hook),hl
        ld a,h
        or l
        ld hl,#0
        ld (.buffer),hl
        jr z,.prepare
        ld hl,#1024
        call __os_malloc
        ld a,d
        or e
        jp z,.oom
        ld (.buffer),de
        ld (.cursor),de
        ld hl,#1024
        add hl,de
        ld (.end),hl
        ld hl,#0
        ld (.count),hl
        ld hl,#.capture
        ld (__esx_print_hook),hl
.prepare:
        ld a,#0xff
        ld (.drive),a
        ld hl,(.command)
        ld a,(hl)
        and #0xdf
        sub #'A'
        cp #2
        jr nc,.execute
        inc hl
        ld a,(hl)
        cp #':'
        jr nz,.execute
        xor a
        call .drive_call
        jr c,.completed
        ld (.drive),a
        ld hl,(.command)
        ld a,(hl)
        and #0xdf
        sub #'A'
        rlca
        rlca
        rlca
        or #0x40
        call .drive_call
        jr c,.completed
        ld hl,(.command)
        inc hl
        inc hl
        ld (.command),hl
.execute:
        ld hl,(.command)
        call __zx_esx_m_execcmd
.completed:
__divmmc_command_return::
        push af
        ld a,(.drive)
        cp #0xff
        call nz,.drive_call
        ld hl,(.hook)
        ld (__esx_print_hook),hl
        ld hl,(.buffer)
        ld a,h
        or l
        jr z,.done
        ld bc,(.count)
        bit 2,b
        jr z,.replay
        ld hl,(.cursor)
.replay:
__divmmc_replay::
        ld a,b
        or c
        jr z,.free
        ld a,(hl)
        push bc
        push hl
        ld hl,(.hook)
        call .invoke
        pop hl
        pop bc
        inc hl
        ld de,(.end)
        or a
        sbc hl,de
        add hl,de
        jr nz,.advance
        ld hl,(.buffer)
.advance:
        dec bc
        jr .replay
.free:
        ld hl,(.buffer)
        call __os_free
.done:
__divmmc_done::
        pop af
        jp _leave_critical_section
.oom:
        ld a,#6                        ; firmware I/O failure, no command ran
        scf
        jp _leave_critical_section
.invoke:
        jp (hl)
.drive_call:
        call __divmmc_firmware
        rst 0x08
        .db 0x89
        jp __divmmc_resume
.capture:
        push af
        push de
        push hl
        ld hl,(.cursor)
        ld (hl),a
        inc hl
        ld de,(.end)
        or a
        sbc hl,de
        add hl,de
        jr nz,.capture_cursor
        ld hl,(.buffer)
.capture_cursor:
        ld (.cursor),hl
        ld hl,(.count)
        bit 2,h
        jr nz,.captured
        inc hl
        ld (.count),hl
.captured:
        pop hl
        pop de
        pop af
        ret
