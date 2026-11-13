        ; Common-memory far call gates.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module partner_yos_far
        .include "native.inc"
        .optsdcc -mz80 sdcccall(1)

        .globl  __native_far20
        .globl  __native_far28
        .globl  __bank_call_iy
        .globl  __partner_bank_select

        .equ __bank_current,0xc002
        .globl __native_current, __native_processes

        .equ    THREAD_CALL_DEPTH, 26
        .equ    THREAD_CALL_STACK, 27
        .equ    BANK_CALL_LIMIT,    8
        .equ    BANK_CALL_SIZE,     5

        .area   _KERNEL_BSS
        ; Only the gateways touch these frames, with bank 1 mapped. Each
        ; process and the UI retain an independent bounded frame stack,
        ; without consuming common RAM or duplicating state in bank 2.
__bank_boot_depth:
        .ds     1
__bank_boot_stack:
        .ds     BANK_CALL_LIMIT*BANK_CALL_SIZE

.process_frames:
        .ds P_COUNT*41
        .area   _CODE

        ; RST 20h descriptor: bank byte followed by little-endian address.
        ; Bit 7 selects a non-returning far jump; clear selects a far call.
        ; AF/BC/DE/HL reach the target unchanged; IX is callee-preserved and
        ; IY is caller-clobbered under the YOS ABI.
__native_far20::
        ex      af,af'
        exx
        pop     iy                      ; inline descriptor
        ld      a,(iy)
        inc     iy
        ld      l,(iy)
        inc     iy
        ld      h,(iy)
        inc     iy
        bit     7,a
        jr      nz,.jump
        push    iy
        pop     de                      ; continuation
        push    hl
        pop     iy                      ; target address
        jp      __bank_call_iy
.jump:
        and     #0x7f
        inc     a                      ; logical bank -> physical bank
        push    hl
        pop     iy
        call    __bank_map
        exx
        ex      af,af'
        jp      (iy)

        ; Dynamic compiler gate. Before RST 28h the caller pushes AF, BC, DE,
        ; HL, then address and bank words. The gate removes that envelope and
        ; invokes the same bank-call core while preserving call arguments.
__native_far28::
        pop     iy                      ; continuation after RST
        pop     bc                      ; C = target bank
        pop     hl                      ; target address
        exx                            ; metadata in alternate register set
        pop     hl
        pop     de
        pop     bc
        pop     af                      ; restore caller arguments
        exx                            ; metadata primary, arguments alternate
        ex      af,af'                  ; preserve caller A in alternate AF
        ld      a,c
        push    iy
        pop     de                      ; continuation
        push    hl
        pop     iy                      ; target
        jp      __bank_call_iy

        ; A = target bank, DE = caller continuation, IY = target, target
        ; argument registers in the alternate set. This routine is entered by
        ; JP, so the target sees only one return address followed immediately
        ; by its ordinary stack arguments.
__bank_call_iy::
        cp      #255                   ; FF keeps the current mapping
        jr      z,.same_bank            ; no bank frame for a common/near call
        inc     a                      ; YOS 0/1 -> Partner 1/2
.bank_number_ready:
        ld      c,a
        ld      a,(__bank_current)
        cp      c
        jr      nz,.cross_bank
.same_bank:
        push    de                      ; direct same-bank return
        exx
        ex      af,af'
        jp      (iy)

.cross_bank:
        push    hl                      ; caller IY (kernel gate); scratch for RST gates
        push    de                      ; continuation, off target stack later
        push    ix
        ld      a,(__bank_current)
        push    af                      ; caller mapping before touching private frames
        ld      a,#1
        call    __bank_map
        call    .depth_address
        ld      a,(hl)
        cp      #BANK_CALL_LIMIT
        jr      nc,.overflow
        inc     (hl)
        ld      e,a
        ld      d,#0
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        inc     hl                      ; depth byte precedes frame array
        pop     af
        ld      (hl),a                  ; caller bank
        inc     hl
        pop     ix
        pop     de
        ld      (hl),e
        inc     hl
        ld      (hl),d                  ; caller continuation
        inc     hl
        pop     de                      ; saved IY
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      a,c
        call    __bank_map
        push    iy                      ; target PC
        push    de
        pop     iy                      ; original kernel caller IY
        pop     hl
        ld      de,#.return
        push    de                      ; target return, then ordinary args
        push    hl
        exx
        ex      af,af'
        ret

.overflow:
        pop     af
        call    __bank_map              ; preserve mapping on rejected call
        pop     ix
        pop     de
        pop     hl
        push    de
        pop     iy                      ; fail safely to the continuation
        exx
        ex      af,af'
        scf
        jp      (iy)

        ; Common return gate. Function result registers are moved to their
        ; alternate set while the caller bank and continuation are restored.
.return:
        ex      af,af'
        exx
        push    ix
        ld      a,#1
        call    __bank_map              ; frames have one copy in the kernel bank
        call    .depth_address
        ld      a,(hl)
        dec     a
        ld      (hl),a
        ld      e,a
        ld      d,#0
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        inc     hl
        ld      c,(hl)                  ; caller bank
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    de                      ; continuation, below saved IX
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    de
        pop     iy                      ; restore kernel caller IY
        ld      a,(__bank_current)
        cp      c
        jr      z,.return_mapped
        ld      a,c
        call    __bank_map
.return_mapped:
        pop     hl                      ; continuation
        pop     ix
        push    hl
        exx
        ex      af,af'
        ret

        ; Output HL = active thread/fallback depth byte, IX = current thread.
        ; Called only while the caller's IX is saved.
.depth_address:
        push bc
        ld hl,(__native_current)
        ld bc,#__native_processes
        or a
        sbc hl,bc
        jr c,.depth_boot
        ld de,#.process_frames
        ld a,#P_COUNT
.depth_next:
        ld b,h
        ld c,l
        push af
        ld a,b
        or c
        jr nz,.depth_nonzero
        pop af
        jr .depth_found
.depth_nonzero:
        pop af
        ld bc,#P_BYTES
        or a
        sbc hl,bc
        jr c,.depth_boot
        push hl
        ld hl,#41
        add hl,de
        ex de,hl
        pop hl
        dec a
        jr nz,.depth_next
.depth_boot:
        ld de,#__bank_boot_depth
.depth_found:
        ex de,hl
        pop bc
        ret

        .globl __native_far30, __far_getb, __far_putb, __alto_call_banked
__native_far30::
        push bc
        push af
        ld a,c
        inc a                          ; FF -> shared, 0/1 -> physical 1/2
        ld c,a
.byte_bank:
        pop af
        jr c,.byte_write
        call __far_getb
        pop bc
        ret
.byte_write:
        call __far_putb
        pop bc
        ret
__alto_call_banked::
        cp #255
        jr z,.near
        push iy
        push af
        push bc
        push de
        push hl
        push bc
        ld l,a
        ld h,#0
        push hl
        rst 0x28
        pop iy
        ret
.near:
        push bc
        ret

__bank_map:
        jp __partner_bank_select

        .area _DATA
        .globl __yos_thread_current_ptr
__yos_thread_current_ptr:: .dw __native_current
