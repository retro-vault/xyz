        ; Close CPC console or AMSDOS firmware descriptors.

        .module close
        .optsdcc -mz80 sdcccall(1)
        .globl  _close
        .globl  __cpc_input_open
        .globl  __cpc_output_open
        .globl  __cpc_cas_in_close
        .globl  __cpc_cas_out_close

CAS_IN_CLOSE    .equ    0xbc7a
CAS_OUT_CLOSE   .equ    0xbc8f

        .area   _CODE
_close::
        ld      a,h
        or      a
        jr      nz,.cpc_close_fail
        ld      a,l
        cp      #3
        jr      c,.cpc_close_ok
        jr      z,.cpc_close_input
        cp      #4
        jr      z,.cpc_close_output
        jr      .cpc_close_fail

.cpc_close_input:
        ld      a,(__cpc_input_open)
        or      a
        jr      z,.cpc_close_fail
        push    ix
        call    __cpc_cas_in_close
        pop     ix
        push    af
        xor     a
        ld      (__cpc_input_open),a
        pop     af
        jr      c,.cpc_close_ok
        jr      .cpc_close_fail

.cpc_close_output:
        ld      a,(__cpc_output_open)
        or      a
        jr      z,.cpc_close_fail
        push    ix
        call    __cpc_cas_out_close
        pop     ix
        push    af
        xor     a
        ld      (__cpc_output_open),a
        pop     af
        jr      c,.cpc_close_ok
        jr      .cpc_close_fail

.cpc_close_ok:
        ld      de,#0
        ret
.cpc_close_fail:
        ld      de,#0xffff
        ret
