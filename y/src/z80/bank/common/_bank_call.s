        ; Common-memory far call gates.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_call
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_call_rst20
        .globl  __bank_call_rst28
        .globl  __bank_call_iy
        .globl  __bank_map
        .globl  __bank_current
        .globl  _thread_current

        .equ    THREAD_CALL_DEPTH, 25
        .equ    THREAD_CALL_STACK, 26
        .equ    BANK_CALL_LIMIT,    4
        .equ    BANK_CALL_SIZE,     3

        .area   _BSS
        ; Kernel-context far calls use the same bounded format. Scheduled
        ; threads keep independent frames in their fixed-memory objects.
__bank_boot_depth:
        .ds     1
__bank_boot_stack:
        .ds     BANK_CALL_LIMIT*BANK_CALL_SIZE

        .area   _CODE

        ; RST 20h descriptor: bank byte followed by little-endian address.
        ; Bit 7 selects a non-returning far jump; clear selects a far call.
        ; AF/BC/DE/HL reach the target unchanged; IX is callee-preserved and
        ; IY is caller-clobbered under the YOS ABI.
__bank_call_rst20::
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
        push    hl
        pop     iy
        call    __bank_map
        exx
        ex      af,af'
        jp      (iy)

        ; Dynamic compiler gate. Before RST 28h the caller pushes AF, BC, DE,
        ; HL, then address and bank words. The gate removes that envelope and
        ; invokes the same bank-call core while preserving call arguments.
__bank_call_rst28::
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
        ld      c,a
        ld      a,(__bank_current)
        cp      c
        jr      nz,.cross_bank
        push    de                      ; direct same-bank return
        exx
        ex      af,af'
        jp      (iy)

.cross_bank:
        push    de                      ; continuation, off target stack later
        push    ix
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
        inc     hl                      ; depth byte precedes frame array
        ld      a,(__bank_current)
        ld      (hl),a                  ; caller bank
        inc     hl
        pop     ix
        pop     de
        ld      (hl),e
        inc     hl
        ld      (hl),d                  ; caller continuation
        ld      a,c
        call    __bank_map
        ld      hl,#.return
        push    hl                      ; target return, then ordinary args
        exx
        ex      af,af'
        jp      (iy)

.overflow:
        pop     ix
        pop     de
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
        call    .depth_address
        ld      a,(hl)
        dec     a
        ld      (hl),a
        ld      e,a
        ld      d,#0
        add     hl,de
        add     hl,de
        add     hl,de
        inc     hl
        ld      c,(hl)                  ; caller bank
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    de
        pop     iy                      ; continuation
        ld      a,(__bank_current)
        cp      c
        jr      z,.return_mapped
        ld      a,c
        call    __bank_map
.return_mapped:
        pop     ix
        exx
        ex      af,af'
        jp      (iy)

        ; Output HL = active thread/fallback depth byte, IX = current thread.
        ; Called only while the caller's IX is saved.
.depth_address:
        ld      ix,(_thread_current)
        push    ix
        pop     hl
        ld      a,h
        or      l
        jr      z,.depth_boot
        ld      de,#THREAD_CALL_DEPTH
        add     hl,de
        ret
.depth_boot:
        ld      hl,#__bank_boot_depth
        ret
