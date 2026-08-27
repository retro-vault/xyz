        ; Reposition the AMSDOS input stream by reopening and skipping.
        ; Firmware output streams are sequential and are not seekable.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek
        .globl  __cpc_input_open
        .globl  __cpc_input_length
        .globl  __cpc_input_length_known
        .globl  __cpc_input_position
        .globl  __cpc_input_name_length
        .globl  __cpc_input_name
        .globl  __cpc_input_buffer
        .globl  __cpc_cas_in_open
        .globl  __cpc_cas_in_close
        .globl  __cpc_cas_in_char

CAS_IN_OPEN     .equ    0xbc77
CAS_IN_CLOSE    .equ    0xbc7a
CAS_IN_CHAR     .equ    0xbc80
SEEK_SET        .equ    0
SEEK_CUR        .equ    1
SEEK_END        .equ    2

        .area   _CODE

        ; _lseek
        ; inputs: HL = fd, 32-bit offset at 4(ix), whence at 8(ix)
        ; outputs: DE:HL = new nonnegative input position, or -1
        ; clobbers: af, bc, de, hl, ix

_lseek::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      a
        jp      nz,.cpc_seek_fail
        ld      a,l
        cp      #3
        jp      nz,.cpc_seek_fail
        ld      a,(__cpc_input_open)
        or      a
        jp      z,.cpc_seek_fail
        ld      a,9(ix)
        or      a
        jp      nz,.cpc_seek_fail
        ld      a,8(ix)
        cp      #SEEK_SET
        jr      z,.cpc_seek_base_zero
        cp      #SEEK_CUR
        jr      z,.cpc_seek_base_cur
        cp      #SEEK_END
        jp      nz,.cpc_seek_fail
        ld      a,(__cpc_input_length_known)
        or      a
        jr      nz,.cpc_seek_base_end
        call    .cpc_seek_measure_end
        jp      nc,.cpc_seek_fail
        jr      .cpc_seek_base_end
.cpc_seek_base_zero:
        ld      hl,#0
        jr      .cpc_seek_add_offset
.cpc_seek_base_cur:
        ld      hl,(__cpc_input_position)
        jr      .cpc_seek_add_offset
.cpc_seek_base_end:
        ld      hl,(__cpc_input_length)

.cpc_seek_add_offset:
        ld      a,7(ix)
        or      a
        jr      z,.cpc_seek_positive_high
        cp      #0xff
        jp      nz,.cpc_seek_fail
        ld      a,6(ix)
        cp      #0xff
        jp      nz,.cpc_seek_fail
        ld      e,4(ix)
        ld      d,5(ix)
        xor     a
        sub     e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        or      a
        sbc     hl,de
        jp      c,.cpc_seek_fail
        jr      .cpc_seek_range
.cpc_seek_positive_high:
        ld      a,6(ix)
        or      a
        jp      nz,.cpc_seek_fail
        ld      e,4(ix)
        ld      d,5(ix)
        add     hl,de
        jp      c,.cpc_seek_fail

.cpc_seek_range:
        ld      de,(__cpc_input_length)
        ld      a,(__cpc_input_length_known)
        or      a
        jr      z,.cpc_seek_store_target
        or      a
        sbc     hl,de
        jp      c,.cpc_seek_in_range
        jp      nz,.cpc_seek_fail
.cpc_seek_in_range:
        add     hl,de
.cpc_seek_store_target:
        ld      (__cpc_seek_target),hl
        ld      de,(__cpc_input_position)
        or      a
        sbc     hl,de
        jr      nc,.cpc_seek_forward

        push    ix
        call    __cpc_cas_in_close
        pop     ix
        jp      nc,.cpc_seek_fail
        ld      hl,#__cpc_input_name
        ld      a,(__cpc_input_name_length)
        ld      b,a
        ld      de,#__cpc_input_buffer
        push    ix
        call    __cpc_cas_in_open
        pop     ix
        jp      nc,.cpc_seek_fail
        xor     a
        ld      (__cpc_input_position),a
        ld      (__cpc_input_position + 1),a

.cpc_seek_forward:
        ld      de,(__cpc_seek_target)
        ld      hl,(__cpc_input_position)
        or      a
        sbc     hl,de
        jr      z,.cpc_seek_done
        push    ix
        call    __cpc_cas_in_char
        pop     ix
        jr      c,.cpc_seek_forward_byte
        jr      z,.cpc_seek_fail
        ld      hl,(__cpc_input_position)
        ld      (__cpc_input_length),hl
        ld      a,#1
        ld      (__cpc_input_length_known),a
        jr      .cpc_seek_fail
.cpc_seek_forward_byte:
        ld      hl,(__cpc_input_position)
        inc     hl
        ld      (__cpc_input_position),hl
        jr      .cpc_seek_forward

        ; Headerless AMSDOS files report a length of zero from CAS IN OPEN.
        ; Measure such a file once when SEEK_END first needs its true end.
.cpc_seek_measure_end:
        push    ix
        call    __cpc_cas_in_char
        pop     ix
        jr      c,.cpc_seek_measure_byte
        jr      z,.cpc_seek_measure_error
        ld      hl,(__cpc_input_position)
        ld      (__cpc_input_length),hl
        ld      a,#1
        ld      (__cpc_input_length_known),a
        scf
        ret
.cpc_seek_measure_byte:
        ld      hl,(__cpc_input_position)
        inc     hl
        ld      a,h
        or      l
        jr      z,.cpc_seek_measure_error
        ld      (__cpc_input_position),hl
        jr      .cpc_seek_measure_end
.cpc_seek_measure_error:
        or      a
        ret

.cpc_seek_done:
        ld      de,(__cpc_seek_target)
        ld      hl,#0
        pop     ix
        ret

.cpc_seek_fail:
        ld      de,#0xffff
        ld      hl,#0xffff
        pop     ix
        ret

        .area   _BSS
__cpc_seek_target:
        .ds     2
