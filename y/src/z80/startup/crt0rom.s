        ; ZX Spectrum 48K ROM reset and restart-vector header.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module crt0rom
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  __startup_init
        .globl  __sys_vec_tbl
        .globl  __sys_stack

        .area   _HEADER

.org    0x0000
.reset:                                ; CPU reset and esxDOS boot handoff
        di
        ; esxDOS returns at 0001 and expects the 48K ROM's XOR A there.
        ; The following jump then enters YOS after firmware has cleared its
        ; boot workspace, so writable data and RAM gates are initialized last.
        xor     a
        jp      .init
        .db     0, 0, 0

        ; divIDE delayed automapping takes over after fetching this opcode.
        ; RST 08 and NMI belong to esxDOS and have no YOS continuation.
.rst08:                                ; esxDOS inline filesystem syscall
        ld      hl, (0x5c5d)
        .db     0, 0, 0, 0, 0

        ; YOS restart vectors dispatch through the writable RAM jump table.
.rst10:                                ; immediate esxDOS boot-text return
        ret
        .db     0, 0, 0, 0, 0, 0, 0

        ; The remaining restart vectors dispatch through writable RAM.
.rst18:                                ; named-service lookup for RAM processes
        jp      __sys_vec_tbl + 6
        .db     0, 0, 0, 0, 0
.rst20:                                ; installable YOS restart handler
        jp      __sys_vec_tbl + 9
        .db     0, 0, 0, 0, 0
.rst28:                                ; installable YOS restart handler
        jp      __sys_vec_tbl + 12
        .db     0, 0, 0, 0, 0
.rst30:                                ; installable YOS restart handler
        jp      __sys_vec_tbl + 15
        .db     0, 0, 0, 0, 0

        ; divIDE executes PUSH AF before paging esxDOS in at 0039. Preserve
        ; the base-ROM return sequence; YOS selects its scheduler with IM2
        ; after disk loading has completed.
.rst38:                                ; esxDOS-compatible IM1 interrupt return
        push    af
        pop     af
        ei
        reti
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0, 0, 0, 0
        .db     0

.nmi:                                  ; esxDOS NMI entry; no YOS handler
        push    af
        .db     0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0
        .db     0, 0, 0, 0, 0

        ; Fixed helper used by esxDOS to read an inline syscall selector
        ; while its firmware is mapped over the application ROM.
.esx_byte_read:
        ld      a, (hl)
        ret

        ; esxDOS completes its cold boot through the conventional 0100h
        ; base-ROM entry. Keep the compatibility header clear up to it.
        .ds     131

        .area   _CODE
.init:
        ld      sp, #0xffff
        call    __startup_init
        ld      sp, #__sys_stack
        im      1
        di                              ; main installs scheduler before EI
        call    _main
.tarpit:
        halt
        jr      .tarpit

        .area   _INITIALIZER
        .area   _INITFINAL
        .area   _GSINIT
        .area   _GSFINAL
        .area   _CONST
        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP
