        ; Read the firmware console or the single AMSDOS input stream.

        .module read
        .optsdcc -mz80 sdcccall(1)
        .globl  _read
        .globl  _getchar
        .globl  __cpc_input_open
        .globl  __cpc_input_position
        .globl  __cpc_input_length
        .globl  __cpc_input_length_known
        .globl  __cpc_cas_in_char

CAS_IN_CHAR     .equ    0xbc80

        .area   _CODE
_read::
        ld      a,h
        or      a
        jp      nz,.cpc_read_fail_plain
        ld      a,l
        or      a
        jr      z,.cpc_read_console
        cp      #3
        jp      nz,.cpc_read_fail_plain
        ld      a,(__cpc_input_open)
        or      a
        jp      z,.cpc_read_fail_plain
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.cpc_file_read_loop:
        ld      a,b
        or      c
        jr      z,.cpc_file_read_done
        call    __cpc_cas_in_char
        jr      c,.cpc_file_read_byte
        jr      z,.cpc_file_read_error
        ld      hl,(__cpc_input_position)
        ld      (__cpc_input_length),hl
        ld      a,#1
        ld      (__cpc_input_length_known),a
        jr      .cpc_file_read_done
.cpc_file_read_byte:
        ld      (hl),a
        inc     hl
        dec     bc
        push    hl
        ld      hl,(__cpc_input_position)
        inc     hl
        ld      (__cpc_input_position),hl
        pop     hl
        jr      .cpc_file_read_loop
.cpc_file_read_error:
        pop     hl
        ld      a,h
        cp      b
        jr      nz,.cpc_file_read_partial
        ld      a,l
        cp      c
        jr      nz,.cpc_file_read_partial
        ld      de,#0xffff
        pop     ix
        ret
.cpc_file_read_partial:
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
.cpc_file_read_done:
        pop     hl
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret

.cpc_read_console:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.cpc_console_read_loop:
        ld      a,b
        or      c
        jr      z,.cpc_console_read_done
        push    bc
        push    hl
        call    _getchar
        ld      a,e
        pop     hl
        pop     bc
        ld      (hl),a
        inc     hl
        dec     bc
        jr      .cpc_console_read_loop
.cpc_console_read_done:
        pop     de
        pop     ix
        ret

.cpc_read_fail_plain:
        ld      de,#0xffff
        ret
