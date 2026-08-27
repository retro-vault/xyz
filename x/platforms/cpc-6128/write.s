        ; Write the firmware console or the single AMSDOS output stream.

        .module write
        .optsdcc -mz80 sdcccall(1)
        .globl  _write
        .globl  __cpc_putchar_a
        .globl  __cpc_output_open
        .globl  __cpc_cas_out_char

CAS_OUT_CHAR    .equ    0xbc95

        .area   _CODE
_write::
        ld      a,h
        or      a
        jr      nz,.cpc_write_fail_plain
        ld      a,l
        cp      #1
        jr      z,.cpc_write_console
        cp      #2
        jr      z,.cpc_write_console
        cp      #4
        jr      nz,.cpc_write_fail_plain
        ld      a,(__cpc_output_open)
        or      a
        jr      z,.cpc_write_fail_plain
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.cpc_file_write_loop:
        ld      a,b
        or      c
        jr      z,.cpc_file_write_done
        ld      a,(hl)
        call    __cpc_cas_out_char
        jr      nc,.cpc_file_write_error
        inc     hl
        dec     bc
        jr      .cpc_file_write_loop
.cpc_file_write_error:
        pop     hl
        ld      a,h
        cp      b
        jr      nz,.cpc_file_write_partial
        ld      a,l
        cp      c
        jr      nz,.cpc_file_write_partial
        ld      de,#0xffff
        pop     ix
        ret
.cpc_file_write_partial:
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
.cpc_file_write_done:
        pop     hl
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret

.cpc_write_console:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.cpc_console_write_loop:
        ld      a,b
        or      c
        jr      z,.cpc_console_write_done
        push    bc
        push    hl
        ld      a,(hl)
        call    __cpc_putchar_a
        pop     hl
        pop     bc
        inc     hl
        dec     bc
        jr      .cpc_console_write_loop
.cpc_console_write_done:
        pop     de
        pop     ix
        ret

.cpc_write_fail_plain:
        ld      de,#0xffff
        ret
