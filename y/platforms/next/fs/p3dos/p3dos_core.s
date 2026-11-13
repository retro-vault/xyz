        ; NextZXOS +3DOS call trampoline and its fixed work area.
        ;
        ; Every filesystem request of the Next build is a +3DOS/IDEDOS call
        ; made directly through the ROM 2 jump table. NextZXOS requires that
        ; during the call
        ;   - MMU0/1 hold ROM, MMU2/3 the default RAM bank 5 (system variables
        ;     and RAM 7 are NextZXOS state and are never written by YOS),
        ;   - MMU4/5 hold RAM bank 2, MMU6/7 RAM bank 7,
        ;   - the stack and every buffer lie in $4000..$BFDF of that map,
        ;   - interrupts run in IM 1.
        ; YOS keeps its kernel data in bank 3 at $4000, so nothing of the
        ; kernel is visible while a call runs: the trampoline and every buffer
        ; live in this area at the top of bank 2 ($B800..$BFDF), which stays
        ; mapped. The caller holds a critical section for the whole call.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module p3dos_core
        .globl  __p3_stack, __p3_stack_top
        .globl  __p3_drive
        .optsdcc -mz80 sdcccall(1)

        .globl  __p3_invoke
        .globl  __p3dos_init
        .globl  __p3_id, __p3_bank, __p3_af, __p3_bc, __p3_de, __p3_hl, __p3_ix
        .globl  __p3_oaf, __p3_obc, __p3_ode, __p3_ohl, __p3_oix
        .globl  __p3_stage, __p3_pa, __p3_pb, __p3_lfn, __p3_cat
        .globl  __p3_ft, __p3_dirs
        .globl  __p3_n, __p3_n2, __p3_dst, __p3_rem, __p3_chunk, __p3_done, __p3_t0, __p3_t1

        .equ    P3_STAGE_SIZE, 512
        .equ    P3_FILES,      16
        .equ    P3_FILE_SIZE,  9
        .equ    P3_DIRS,       8

        .area   _P3DOS

        ; Call parameter block: inputs, then results.
__p3_id:        .dw 0           ; +3DOS call id (CALL address)
__p3_bank:      .db 7           ; reserved (legacy hook bank)
.p3_save8e:     .db 0
__p3_af:        .dw 0           ; A,F in    (A = high byte)
__p3_bc:        .dw 0           ; BC in   (B = high byte)
__p3_de:        .dw 0           ; DE in
__p3_hl:        .dw 0           ; HL in
__p3_ix:        .dw 0           ; IX in
__p3_oaf:       .dw 0           ; AF out    (carry set = success)
__p3_obc:       .dw 0           ; BC out
__p3_ode:       .dw 0           ; DE out
__p3_ohl:       .dw 0           ; HL out
__p3_oix:       .dw 0           ; IX out

        ; Scratch words shared by the adaptors (one call at a time).
__p3_drive:     .db 'C'         ; active +3DOS drive for relative paths
__p3_n:         .db 0
__p3_n2:        .db 0
__p3_dst:       .dw 0
__p3_rem:       .dw 0
__p3_chunk:     .dw 0
__p3_done:      .dw 0
__p3_t0:        .dw 0
__p3_t1:        .dw 0

        ; Saved machine state while a call runs.
.p3_savesp:     .dw 0
.p3_save56:     .db 0
.p3_save57:     .db 0
.p3_savei:      .db 0
.p3_pad2:       .db 0

        ; Data areas. Files: used, pos(4), size(4). Directories: see
        ; native_calls.s.
__p3_ft:        .ds P3_FILES*P3_FILE_SIZE
        ; Native catalog state is allocated only for live directories.
        ; This pointer table replaces two permanently reserved 361-byte slots.
__p3_dirs:      .ds P3_DIRS*2
__p3_pa:        .ds 260         ; first +3DOS path ($FF terminated)
__p3_cat:       .ds 13*7        ; DOS_CATALOG buffer: preload + 6 entries
        ; The second rename path and the IDE_GET_LFN name buffer are never
        ; needed while file data is staged, so all three share one buffer.
__p3_pb:
__p3_lfn:
__p3_stage:     .ds P3_STAGE_SIZE

        ; The OS runs on this private stack.
__p3_stack:      .ds 512
__p3_stack_top:

        ; Paint the private ROM stack for headroom checks, then reset handles.
__p3dos_init:
        ld hl,#__p3_stack
        ld de,#__p3_stack+1
        ld bc,#511
        ld (hl),#0xa5
        ldir
        ld      hl,#__p3_ft
        ld      de,#__p3_ft+1
        ld      bc,#P3_FILES*P3_FILE_SIZE-1
        ld      (hl),#0
        ldir
        ld      hl,#__p3_dirs
        ld      de,#__p3_dirs+1
        ld      bc,#P3_DIRS*2-1
        ld      (hl),#0
        ldir
        ret

        ; Make one +3DOS call from the parameter block. Interrupts are
        ; already disabled (critical section). Preserves IX and IY; every
        ; other register is clobbered.
__p3_invoke:
        push    ix
        push    iy
        ld      (.p3_savesp),sp
        ld      sp,#__p3_stack_top
        ld      a,i
        ld      (.p3_savei),a
        ; remember what is mapped at $C000
        ld      bc,#0x243b
        ld      a,#0x56
        out     (c),a
        inc     b
        in      a,(c)
        ld      (.p3_save56),a
        dec     b
        ld      a,#0x57
        out     (c),a
        inc     b
        in      a,(c)
        ld      (.p3_save57),a
        ; Select NextZXOS ROM 2 and its workspace bank 7, preserving the
        ; paging latch. Writing 8E also establishes the standard ROM/RAM map.
        ld      bc,#0x243b
        ld      a,#0x8e
        out     (c),a
        inc     b
        in      a,(c)
        ld      (.p3_save8e),a
        ld      de,#0x8e7a
        call    .p3_nr
        ; NextZXOS normal configuration: ROM 2, RAM5, RAM2, RAM7
        ld      de,#0x50ff
        call    .p3_nr
        ld      de,#0x51ff
        call    .p3_nr
        ld      de,#0x520a
        call    .p3_nr
        ld      de,#0x530b
        call    .p3_nr
        ld      de,#0x560e
        call    .p3_nr
        ld      de,#0x570f
        call    .p3_nr
        ; NextZXOS's ROM routines use programmable firmware traps internally.
        ; Enable them only while the OS is visible; YOS uses its own vectors.
        ld      bc,#0x243b
        ld      a,#0x0a
        out     (c),a
        inc     b
        in      a,(c)
        or      #0x10
        out     (c),a
        ; A real CALL through a self-modified operand in common bank 2.
        ld      hl,(__p3_id)
        ld      (.p3_call+1),hl
        ld      ix,(__p3_ix)
        ld      bc,(__p3_bc)
        ld      de,(__p3_de)
        ld      hl,(__p3_af)
        push    hl
        pop     af
        ld      hl,(__p3_hl)
        ld      iy,#0x5c3a
        im      1
.p3_call:
        call    0x0100
        di                              ; the OS may have enabled interrupts
        im      2
        ; results
        ld      (__p3_ohl),hl
        ld      (__p3_ode),de
        ld      (__p3_obc),bc
        push    af
        pop     hl
        ld      (__p3_oaf),hl
        ld      (__p3_oix),ix
        ld      bc,#0x243b
        ld      a,#0x0a
        out     (c),a
        inc     b
        in      a,(c)
        and     #0xef
        out     (c),a
        ld      bc,#0x00e3
        xor     a
        out     (c),a
        ; Restore the paging latch before restoring YOS's explicit MMU map.
        ld      a,(.p3_save8e)
        ld      e,a
        ld      d,#0x8e
        call    .p3_nr
        ; back to the YOS map
        ld      de,#0x5008
        call    .p3_nr
        ld      de,#0x5109
        call    .p3_nr
        ld      de,#0x5206
        call    .p3_nr
        ld      de,#0x5307
        call    .p3_nr
        ld      a,(.p3_save56)
        ld      e,a
        ld      d,#0x56
        call    .p3_nr
        ld      a,(.p3_save57)
        ld      e,a
        ld      d,#0x57
        call    .p3_nr
        ld      a,(.p3_savei)
        ld      i,a
        ld      sp,(.p3_savesp)
        pop     iy
        pop     ix
        ret

        ; D = NextReg, E = value.
.p3_nr:
        ld      bc,#0x243b
        out     (c),d
        inc     b
        out     (c),e
        ret
