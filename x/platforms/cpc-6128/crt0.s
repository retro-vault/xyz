        ; CPC 6128 firmware-hosted C program startup.
        ; Uses the lower memory pool and returns to BASIC on exit.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _exit
        .globl  _entry
        .globl  s__BSS
        .globl  l__BSS
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER
        .globl  __cpc_cas_in_close

CPC_STACK_TOP   .equ    0xa6fc

        .area   _CODE
_entry::
        ld      hl,#0
        add     hl,sp
        ld      sp,#CPC_STACK_TOP
        push    hl
        call    .gsinit
        call    .cpc_select_disc
        ; RUN transfers control while its input stream is still open. Close
        ; the loader channel before application code starts using AMSDOS.
        push    ix
        call    __cpc_cas_in_close
        pop     ix
        call    _main
        ex      de,hl
        call    _exit
.cpc_start_halt:
        halt
        jr      .cpc_start_halt

        ; The CPC firmware routes cassette input and output independently.
        ; Make a disk platform deterministic even if BASIC selected tape.
.cpc_select_disc:
        push    ix
        xor     a
        rst     #0x18
        .dw     .cpc_disc_far
        pop     ix
        ret

.cpc_disc_trampoline:
        ld      iy,(#0xbe7d)
        jp      0xccd1

        .area   _CONST
.cpc_disc_far:
        .dw     .cpc_disc_trampoline   ; AMSDOS |DISC handler via RAM
        .db     7                      ; standard AMSDOS upper ROM slot

        .area   _GSINIT
.gsinit:
        ld      bc,#l__BSS
        ld      a,b
        or      c
        jr      z,.cpc_no_bss
        ld      hl,#s__BSS
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      c
        jr      z,.cpc_no_bss
        ldir
.cpc_no_bss:
        ld      de,#s__INITIALIZED
        ld      hl,#s__INITIALIZER
        ld      bc,#l__INITIALIZER
        ld      a,b
        or      c
        jr      z,.cpc_no_init
        ldir
.cpc_no_init:
        .area   _GSFINAL
        ret

        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _INITIALIZER
        .area   _HEAP
