        ; Rename an AMSDOS file through its firmware RSX command.

        .module rename
        .optsdcc -mz80 sdcccall(1)
        .globl  _rename
        .globl  __cpc_input_open
        .globl  __cpc_output_open
        .globl  __cpc_input_buffer
        .globl  __cpc_cas_in_open
        .globl  __cpc_cas_in_close

        .area   _CODE
_rename::
        ld      a,(__cpc_input_open)
        or      a
        jp      nz,.cpc_rename_fail
        ld      a,(__cpc_output_open)
        or      a
        jp      nz,.cpc_rename_fail
        ld      a,h
        or      l
        jp      z,.cpc_rename_fail
        ld      a,d
        or      e
        jp      z,.cpc_rename_fail
        ld      (__cpc_ren_old_desc + 1),hl
        ld      (__cpc_ren_new_desc + 1),de
        push    de
        push    hl
        call    .cpc_rename_length
        pop     hl
        pop     de
        jp      nc,.cpc_rename_fail
        ld      a,c
        ld      (__cpc_ren_old_desc),a
        ex      de,hl
        push    de
        call    .cpc_rename_length
        pop     de
        jp      nc,.cpc_rename_fail
        ld      a,c
        ld      (__cpc_ren_new_desc),a
        ; RSX parameters are stored in reverse source order: the old name
        ; (the second |REN argument) is at IX+0, then the new name at IX+2.
        ld      hl,#__cpc_ren_old_desc
        ld      (__cpc_ren_params),hl
        ld      hl,#__cpc_ren_new_desc
        ld      (__cpc_ren_params + 2),hl

        ; Reject a missing source or an existing destination before invoking
        ; the status-less AMSDOS RSX entry point.
        ld      hl,#__cpc_ren_old_desc
        call    .cpc_rename_exists
        jp      nc,.cpc_rename_fail
        ld      hl,#__cpc_ren_new_desc
        call    .cpc_rename_exists
        jp      c,.cpc_rename_fail

        push    ix
        ld      ix,#__cpc_ren_params
        ld      a,#2
        rst     #0x18
        .dw     .cpc_ren_far
        pop     ix

        ; Verify both names so the POSIX-style return value reflects the ROM
        ; operation rather than the RSX dispatch itself.
        ld      hl,#__cpc_ren_old_desc
        call    .cpc_rename_exists
        jp      c,.cpc_rename_fail
        ld      hl,#__cpc_ren_new_desc
        call    .cpc_rename_exists
        jp      nc,.cpc_rename_fail
        ld      de,#0
        ret

.cpc_ren_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xd4c4

        ; Entry: HL points at a BASIC string descriptor.
        ; Exit: carry set only when the file could be opened and closed.
.cpc_rename_exists:
        ld      b,(hl)
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      de,#__cpc_input_buffer
        push    ix
        call    __cpc_cas_in_open
        pop     ix
        ret     nc
        push    ix
        call    __cpc_cas_in_close
        pop     ix
        ret

.cpc_rename_length:
        ld      c,#0
.cpc_rename_length_loop:
        ld      a,(hl)
        or      a
        jr      z,.cpc_rename_length_end
        inc     hl
        inc     c
        ld      a,c
        cp      #17
        jr      c,.cpc_rename_length_loop
        or      a
        ret
.cpc_rename_length_end:
        ld      a,c
        or      a
        ret     z
        scf
        ret

.cpc_rename_fail:
        ld      de,#0xffff
        ret

        .area   _CONST
.cpc_ren_far:
        .dw     .cpc_ren_trampoline    ; AMSDOS |REN handler via RAM
        .db     7                      ; standard AMSDOS upper ROM slot

        .area   _BSS
__cpc_ren_params:
        .ds     4
__cpc_ren_old_desc:
        .ds     3
__cpc_ren_new_desc:
        .ds     3
