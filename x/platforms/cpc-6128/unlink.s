        ; Erase an AMSDOS file through its firmware RSX command.

        .module unlink
        .optsdcc -mz80 sdcccall(1)
        .globl  _unlink
        .globl  __cpc_input_open
        .globl  __cpc_output_open
        .globl  __cpc_input_buffer
        .globl  __cpc_cas_in_open
        .globl  __cpc_cas_in_close

        .area   _CODE
_unlink::
        ld      a,(__cpc_input_open)
        or      a
        jr      nz,.cpc_unlink_fail
        ld      a,(__cpc_output_open)
        or      a
        jr      nz,.cpc_unlink_fail
        ld      a,h
        or      l
        jr      z,.cpc_unlink_fail
        ld      (__cpc_era_desc + 1),hl
        push    hl
        call    .cpc_unlink_length
        pop     hl
        jr      nc,.cpc_unlink_fail
        ld      a,c
        ld      (__cpc_era_desc),a
        ld      hl,#__cpc_era_desc
        ld      (__cpc_era_params),hl

        ld      hl,#__cpc_era_desc
        call    .cpc_unlink_exists
        jr      nc,.cpc_unlink_fail
        push    ix
        ld      ix,#__cpc_era_params
        ld      a,#1
        rst     #0x18
        .dw     .cpc_era_far
        pop     ix
        ld      hl,#__cpc_era_desc
        call    .cpc_unlink_exists
        jr      c,.cpc_unlink_fail
        ld      de,#0
        ret

.cpc_era_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xd48a

.cpc_unlink_exists:
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

.cpc_unlink_length:
        ld      c,#0
.cpc_unlink_length_loop:
        ld      a,(hl)
        or      a
        jr      z,.cpc_unlink_length_end
        inc     hl
        inc     c
        ld      a,c
        cp      #17
        jr      c,.cpc_unlink_length_loop
        or      a
        ret
.cpc_unlink_length_end:
        ld      a,c
        or      a
        ret     z
        scf
        ret

.cpc_unlink_fail:
        ld      de,#0xffff
        ret

        .area   _CONST
.cpc_era_far:
        .dw     .cpc_era_trampoline    ; AMSDOS |ERA handler via RAM
        .db     7                      ; standard AMSDOS upper ROM slot

        .area   _BSS
__cpc_era_params:
        .ds     2
__cpc_era_desc:
        .ds     3
