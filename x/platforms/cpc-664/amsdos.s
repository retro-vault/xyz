        ; Direct entry points for the stock AMSDOS ROM in slot 7.
        ; Bypassing its patched jumpblock thunks keeps ordinary C stack
        ; frames out of AMSDOS's special CAS dispatcher protocol.

        .module amsdos

        .globl  __cpc_cas_in_open
        .globl  __cpc_cas_in_close
        .globl  __cpc_cas_in_char
        .globl  __cpc_cas_out_open
        .globl  __cpc_cas_out_close
        .globl  __cpc_cas_out_char

        .area   _CODE
__cpc_cas_in_open::
        rst     #0x18
        .dw     .cpc_cas_in_open_far
        ret
__cpc_cas_in_close::
        rst     #0x18
        .dw     .cpc_cas_in_close_far
        ret
__cpc_cas_in_char::
        rst     #0x18
        .dw     .cpc_cas_in_char_far
        ret
__cpc_cas_out_open::
        rst     #0x18
        .dw     .cpc_cas_out_open_far
        ret
__cpc_cas_out_close::
        rst     #0x18
        .dw     .cpc_cas_out_close_far
        ret
__cpc_cas_out_char::
        rst     #0x18
        .dw     .cpc_cas_out_char_far
        ret

        ; AMSDOS is a foreground ROM, so the kernel's direct far-call path
        ; does not supply its IY workspace pointer. Enter through RAM while
        ; ROM 7 is selected, load the published workspace pointer, and then
        ; jump to the corresponding ROM operation.
.cpc_cas_in_open_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xcd4c
.cpc_cas_in_close_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xcd4f
.cpc_cas_in_char_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xcd55
.cpc_cas_out_open_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xcd61
.cpc_cas_out_close_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xcd64
.cpc_cas_out_char_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xcd6a

        .area   _CONST
.cpc_cas_in_open_far:
        .dw     .cpc_cas_in_open_trampoline
        .db     7
.cpc_cas_in_close_far:
        .dw     .cpc_cas_in_close_trampoline
        .db     7
.cpc_cas_in_char_far:
        .dw     .cpc_cas_in_char_trampoline
        .db     7
.cpc_cas_out_open_far:
        .dw     .cpc_cas_out_open_trampoline
        .db     7
.cpc_cas_out_close_far:
        .dw     .cpc_cas_out_close_trampoline
        .db     7
.cpc_cas_out_char_far:
        .dw     .cpc_cas_out_char_trampoline
        .db     7
