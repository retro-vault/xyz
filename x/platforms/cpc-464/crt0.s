        ; CPC 464 firmware-hosted C program startup.
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

CPC_STACK_TOP   .equ    0xa6fc

        .area   _CODE

        ; _entry
        ; inputs: SP points at the firmware/BASIC return address
        ; outputs: does not return through the ordinary C call chain
        ; clobbers: af, bc, de, hl

_entry::
        ld      hl,#0
        add     hl,sp
        ld      sp,#CPC_STACK_TOP
        push    hl
        call    .gsinit
        call    _main
        ex      de,hl
        call    _exit
.cpc_start_halt:
        halt
        jr      .cpc_start_halt

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
