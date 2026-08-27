        ; Open one AMSDOS input or output stream through firmware.
        ; Descriptors 3 and 4 name the input and output channels.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open
        .globl  __cpc_input_open
        .globl  __cpc_output_open
        .globl  __cpc_input_length
        .globl  __cpc_input_length_known
        .globl  __cpc_input_position
        .globl  __cpc_input_name_length
        .globl  __cpc_input_name
        .globl  __cpc_input_buffer
        .globl  __cpc_output_buffer
        .globl  __cpc_cas_in_open
        .globl  __cpc_cas_out_open

CAS_IN_OPEN     .equ    0xbc77
CAS_OUT_OPEN    .equ    0xbc8c
O_ACCMODE       .equ    0x03
O_APPEND_HI     .equ    0x04

        .area   _CODE

        ; _open
        ; inputs: HL = NUL-terminated name, DE = open flags
        ; outputs: DE = fd 3 (input), fd 4 (output), or -1
        ; clobbers: af, bc, de, hl

_open::
        ld      a,h
        or      l
        jp      z,.cpc_open_fail
        ld      a,d
        and     #O_APPEND_HI
        jp      nz,.cpc_open_fail
        ld      a,e
        and     #O_ACCMODE
        jr      z,.cpc_open_input
        cp      #1
        jr      z,.cpc_open_output
        jp      .cpc_open_fail

.cpc_open_input:
        ld      a,(__cpc_input_open)
        or      a
        jp      nz,.cpc_open_fail
        push    hl
        call    .cpc_name_length
        pop     hl
        jp      nc,.cpc_open_fail
        ld      a,c
        ld      (__cpc_input_name_length),a
        push    bc
        push    hl
        ld      de,#__cpc_input_name
        ld      b,#0
        ldir
        pop     hl
        pop     bc
        ld      b,c
        ld      de,#__cpc_input_buffer
        push    ix
        call    __cpc_cas_in_open
        pop     ix
        jp      nc,.cpc_open_fail
        ld      (__cpc_input_length),bc
        ld      a,b
        or      c
        jr      z,.cpc_open_unknown_length
        ld      a,#1
        jr      .cpc_open_store_length_state
.cpc_open_unknown_length:
        xor     a
.cpc_open_store_length_state:
        ld      (__cpc_input_length_known),a
        xor     a
        ld      (__cpc_input_position),a
        ld      (__cpc_input_position + 1),a
        inc     a
        ld      (__cpc_input_open),a
        ld      de,#3
        ret

.cpc_open_output:
        ld      a,(__cpc_output_open)
        or      a
        jp      nz,.cpc_open_fail
        push    hl
        call    .cpc_name_length
        pop     hl
        jp      nc,.cpc_open_fail
        ld      b,c
        ld      de,#__cpc_output_buffer
        push    ix
        call    __cpc_cas_out_open
        pop     ix
        jp      nc,.cpc_open_fail
        ld      a,#1
        ld      (__cpc_output_open),a
        ld      de,#4
        ret

        ; Return C = filename length and carry set for 1..16 bytes.
.cpc_name_length:
        ld      c,#0
.cpc_name_loop:
        ld      a,(hl)
        or      a
        jr      z,.cpc_name_end
        inc     hl
        inc     c
        ld      a,c
        cp      #17
        jr      c,.cpc_name_loop
        or      a
        ret
.cpc_name_end:
        ld      a,c
        or      a
        ret     z
        scf
        ret

.cpc_open_fail:
        ld      de,#0xffff
        ret
